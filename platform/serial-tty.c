/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A platform that speaks over a Unix TTY.
 */
#define _POSIX_C_SOURCE 200809L
#include "serial.h"
#include "platform-posix.h"
#include "debug.h"
//#include "ax25_dl.h"
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <fcntl.h>
#include <netdb.h>
#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum { MAX_SERIAL = 3 };

void kiss_recv_byte(uint8_t port, uint8_t byte);

static int serial_fd[MAX_SERIAL] = {-1,-1,-1};

static void setup_set_raw(int fd) {
    struct termios tbuf;

    if (tcgetattr(fd, &tbuf) == -1) {
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


    if (tcsetattr(fd, TCSANOW, &tbuf) == -1) {
        panic("tcsetattr");
    }
}

static void serial_init_pty(int *fd) {
    int other_fd;
    char other_name[1024];

    if (openpty(fd, &other_fd, NULL, NULL, NULL) == -1)
        panic("openpty failed");

    if (ttyname_r(other_fd, other_name, sizeof(other_name)) == -1)
        panic("failed to get ttyname");

    DEBUG(STR("terminal: "), STR(other_name));
}

static void serial_init_external(int *fd, const char *tty) {
    *fd = open(tty, O_RDWR);
    if (*fd == -1)
        panic("failed to open port");
    DEBUG(STR("terminal: "), STR(tty));
}

void serial_putch(uint8_t serial, uint8_t data) {
    CHECK(serial < MAX_SERIAL);
    if (write(serial_fd[serial], &data, sizeof(data)) == -1)
        panic("cannot write");
}

static void serial_got_ch(int serial) {
    uint8_t data;
    if (read(serial_fd[serial], &data, sizeof(data)) != 1)
        panic("cannot read");
    serial_recv_byte(serial, data);
    //kiss_recv_byte(serial, data);
}

static void serial_got_ch0(void) { serial_got_ch(0); }
static void serial_got_ch1(void) { serial_got_ch(2); }
static void serial_got_ch2(void) { serial_got_ch(2); }

static fd_event_t serial_fd_event[MAX_SERIAL] = {
    { .next = NULL, .fd = -1, .callback = serial_got_ch0, },
    { .next = NULL, .fd = -1, .callback = serial_got_ch1, },
    { .next = NULL, .fd = -1, .callback = serial_got_ch2, },
};

void serial_init(int argc, char *argv[]) {
    /* Open up N serial ports */
    for (size_t i = 0; i < MAX_SERIAL; ++i) {
        if (argc < (ssize_t)i + 2) {
            if (i == MAX_SERIAL - 1) {
                serial_init_external(&serial_fd[i], "/dev/tty");
            } else {
                serial_init_pty(&serial_fd[i]);
                setup_set_raw(serial_fd[i]);
            }
        } else {
            serial_init_external(&serial_fd[i], argv[i+1]);
            setup_set_raw(serial_fd[i]);
        }
        serial_fd_event[i].fd = serial_fd[i];
        register_fd_event(&serial_fd_event[i]);
    }
}
