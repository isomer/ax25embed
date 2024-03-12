#include "platform.h"
#include "kiss.h"
#include "serial.h"
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
    tbuf.c_oflag &= ~OPOST; /* disable output postprocessing */
    tbuf.c_lflag &= ~(ICANON | ISIG | ECHO); /* disable input canonicalisation, signal processing, local echo */
    tbuf.c_cc[VMIN] = 1; /* allow byte by byte */
    tbuf.c_cc[VTIME] = 0; /* don't delay */

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

int main(int argc, char *argv[]) {
    debug("Initializing");
    if (argc > 1) {
        serial_init_external(argv[1]);
    } else {
        serial_init_internal();
    }
    debug("Running");
    for (;;) {
        uint8_t data;
        if (read(serial_fd, &data, sizeof(data)) != 1)
            panic("cannot read");
        kiss_recv_byte(0, data);
    }
}

