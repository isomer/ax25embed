/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A platform that speaks over a Unix TTY.
 */
#include "platform.h"
#include "connection.h"
#include "kiss.h"
#include "serial.h"
#include "ax25_dl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <fcntl.h>
#include <netdb.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define NODE "::1"
#define SERVICE "8001"

static int serial_fd = -1;

static void serial_init_internal(void) {
    int master_fd;

    int four = 4; // This value appears unused in the kernel?
    int kiss = N_AX25;
    char call[7] = "2E0ITB\x00";
    struct ifreq ifr;

    if (openpty(&master_fd, &serial_fd, NULL, NULL, NULL) == -1)
        panic("openpty failed");

    if (ioctl(master_fd, TIOCSETD, &kiss) == -1)
        panic("ioctl(TIOCSETD)");

    for(size_t i=0; i<sizeof(call); ++i)
        call[i] <<= 1;

    if (ioctl(master_fd, SIOCSIFHWADDR, call) == -1)
        panic("ioctl(SIOCSIFHWADDR)");

    if (ioctl(master_fd, SIOCSIFENCAP, &four) == -1)
        panic("ioctl(SIOCSIFENCAP)");

    if (ioctl(master_fd, SIOCGIFNAME, ifr.ifr_name) == -1)
        panic("ioctl(SIOCGIFNAME)");

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == -1)
        panic("socket(AF_UNIX)");

    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == -1)
        panic("ioctl(SIOCGIFLAGS)");

    ifr.ifr_flags |= IFF_UP | IFF_RUNNING | IFF_NOARP;
    ifr.ifr_flags &= ~IFF_BROADCAST;

    if (ioctl(fd, SIOCSIFFLAGS, &ifr) == -1)
        panic("ioctl(SIOCSIFLAGS)");

    close(fd);
}

static void serial_init_external(const char *tty) {
    struct termios tbuf;

    serial_fd = open(tty, O_RDWR);
    if (serial_fd == -1)
        panic("failed to open port");

    if (tcgetattr(serial_fd, &tbuf) == -1) {
        panic("tcgetattr");
    }
    //tbuf.c_oflag &= ~OPOST; /* disable output postprocessing */
    //tbuf.c_lflag &= ~(ICANON | ISIG | ECHO); /* disable input canonicalisation, signal processing, local echo */
    tbuf.c_cc[VMIN] = 1; /* allow byte by byte */
    tbuf.c_cc[VTIME] = 0; /* don't delay */

    tbuf.c_iflag &= ~(IGNBRK /* Ignore Break */
            | BRKINT /* Break interrupts */
            | PARMRK /* Mark parity errors */
            | ISTRIP /* Strip off 8th bit */
            | INLCR /* Translate NL to CR on input */
            | IGNCR /* Ignore input CR */
            | ICRNL
            | IXON /* Input XON/XOFF processing on output */
            );
    tbuf.c_oflag &= ~OPOST /* Disable post processing */;
    tbuf.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tbuf.c_cflag &= ~(CSIZE | PARENB);
    tbuf.c_cflag |= CS8;


    if (tcsetattr(serial_fd, TCSANOW, &tbuf) == -1) {
        panic("tcsetattr");
    }
}

void serial_putch(uint8_t serial, uint8_t data) {
    if (serial == 0) {
        if (write(serial_fd, &data, sizeof(data)) == -1)
            panic("cannot write");
    }
}

static void soc_error(dl_socket_t *sock, ax25_dl_error_t err) {
    (void) sock;
    DEBUG(STR("Got error="), STR(ax25_dl_strerror(err)));
}

static void soc_data(dl_socket_t *sock, uint8_t *data, size_t datalen) {
    (void) sock;
    (void) data;
    (void) datalen;
    DEBUG(STR("Got data, len="), D8(datalen));
    for(size_t i = 0; i < datalen; ++i) {
        if ((data[i] >= 'A' && data[i] <= 'Z') || (data[i] >= 'a' && data[i] <= 'z')) {
            data[i] ^= ('a' - 'A');
        }
    }
    dl_send(sock, data, datalen);
}

static void soc_disconnect(dl_socket_t *sock) {
    (void) sock;
    DEBUG(STR("disconnect"));
}

static void soc_connect(dl_socket_t *sock) {
    DEBUG(STR("connected"));
    sock->on_error = soc_error;
    sock->on_data = soc_data;
    sock->on_disconnect = soc_disconnect;
}


int main(int argc, char *argv[]) {
    debug("Initializing");
    if (argc > 1) {
        serial_init_external(argv[1]);
    } else {
        serial_init_internal();
    }
    listen_socket.on_connect = soc_connect;
    debug("Running");
    for (;;) {
        uint8_t data;
        fd_set rfd;
        FD_ZERO(&rfd);
        FD_SET(serial_fd, &rfd);
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 10000, };
        select(serial_fd+1, &rfd, NULL, NULL, &timeout);
        if (FD_ISSET(serial_fd, &rfd)) {
            if (read(serial_fd, &data, sizeof(data)) != 1)
                panic("cannot read");
            kiss_recv_byte(0, data);
        }
        conn_expire_timers();
        conn_dequeue();
    }
}

