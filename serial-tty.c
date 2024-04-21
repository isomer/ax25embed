/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A platform that speaks over a Unix TTY.
 */
#define _POSIX_C_SOURCE 200809L
#include "platform.h"
#include "connection.h"
#include "kiss.h"
#include "serial.h"
#include "ax25_dl.h"
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


static int serial_fd = -1;

static void setup_serial(int fd) {
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

static void serial_init_internal(void) {
    int other_fd;
    char other_name[1024];

    if (openpty(&serial_fd, &other_fd, NULL, NULL, NULL) == -1)
        panic("openpty failed");

    if (ttyname_r(other_fd, other_name, sizeof(other_name)) == -1)
        panic("failed to get ttyname");

    DEBUG(STR("terminal: "), STR(other_name));

    setup_serial(serial_fd);
}

static void serial_init_external(const char *tty) {
    serial_fd = open(tty, O_RDWR);
    if (serial_fd == -1)
        panic("failed to open port");

    setup_serial(serial_fd);
}

void serial_putch(uint8_t serial, uint8_t data) {
    if (serial == 0) {
        if (write(serial_fd, &data, sizeof(data)) == -1)
            panic("cannot write");
    }
}

void serial_init(int argc, char *argv[]) {
    if (argc > 1) {
        serial_init_external(argv[1]);
    } else {
        serial_init_internal();
    }
}

void serial_wait(void) {
    duration_t wait = DURATION_ZERO;
    for (;;) {
        uint8_t data;
        fd_set rfd;
        FD_ZERO(&rfd);
        FD_SET(serial_fd, &rfd);
        wait = conn_expire_timers();
        int64_t wait_us = duration_as_micros(wait);
        struct timeval timeout = { .tv_sec = wait_us / 1000000, .tv_usec = wait_us % 1000000 };
        select(serial_fd+1, &rfd, NULL, NULL, &timeout);
        if (FD_ISSET(serial_fd, &rfd)) {
            if (read(serial_fd, &data, sizeof(data)) != 1)
                panic("cannot read");
            kiss_recv_byte(0, data);
        }
        conn_dequeue();
    }
}

