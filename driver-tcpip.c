#include "platform.h"
#include "kiss.h"
#include "serial.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define NODE "::1"
#define SERVICE "8001"

static int serial_fd = -1;

void debug_putch(char ch) {
    putchar(ch);
}

void debug(const char *msg) {
    puts(msg);
}

void panic(const char *msg) {
    debug(msg);
    abort();
}

void serial_init(void) {
    int listenfd = -1;
    struct addrinfo *addrs = NULL;
    int r = getaddrinfo(NODE, SERVICE, NULL, &addrs);
    if (r == -1)
        panic("cannot lookup address");
    for (struct addrinfo *rp = addrs; rp != NULL; rp = rp->ai_next) {
        listenfd = socket(rp->ai_family, rp->ai_socktype,
                rp->ai_protocol);
        if (listenfd == -1)
            continue;

        const int one = 1;
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

        if (bind(listenfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;                  /* Success */

        close(listenfd);
    }

    if (listenfd == -1)
        panic("cannot bind");

    freeaddrinfo(addrs);

    if (listen(listenfd, 1) == -1)
        panic("cannot listen");

    if ((serial_fd = accept(listenfd, NULL, NULL)) == -1)
        panic("cannot accept");
}

void serial_putch(uint8_t serial, uint8_t data) {
    if (serial == 0) {
        if (write(serial_fd, &data, sizeof(data)) == -1)
            panic("cannot write");
    }
}

void serial_wait(void) {
    for (;;) {
        uint8_t data;
        if (read(serial_fd, &data, sizeof(data)) != 1)
            panic("cannot read");
        kiss_recv_byte(0, data);
    }
}
