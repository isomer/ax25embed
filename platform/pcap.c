/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * PCAP(ng) writer for posix systems
 */
#include "platform-posix.h"
#include "capture.h"
#include "clock.h"
#include "debug.h"
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

enum {
    PCAP_MAGIC = 0x0A0D0D0A,
    PCAP_BYTE_ORDER_MAGIC = 0x1A2B3C4D,
    PCAP_MAJOR = 1,
    PCAP_MINOR = 0,
    PCAP_TYPE_IDB = 1, /* Interface Description Block */
    PCAP_TYPE_EPB = 2, /* Enhanced Packet Block */
    PCAP_NO_SNAPLEN = 0,
};

enum {
    LINKTYPE_AX25 = 3,
};

typedef enum {
    PCAP_OPT_END = 0,
    PCAP_OPT_COMMENT = 1,
    PCAP_OPT_SHB_HARDWARE = 2,
    PCAP_OPT_SHB_OS = 3,
    PCAP_OPT_SHB_USER_APPL = 4,
    PCAP_OPT_FLAGS = 2,
} pcap_option_t;

enum {
    PCAP_FLAG_UNKNOWN = 0b00,
    PCAP_FLAG_IN  = 0b01,
    PCAP_FLAG_OUT = 0b10,

    PCAP_FLAG_UNICAST   = 0b00100,
    PCAP_FLAG_MULTICAST = 0b01000,
    PCAP_FLAG_BROADCAST = 0b01100,
    PCAP_FLAG_PROMISC   = 0b10000,
};

static int posix_fd = -1;

static void pcap_write(const void *data, size_t datalen) {
    write(posix_fd, data, datalen);
}

static void pcap_write_padded(const void *data, size_t datalen) {
    static uint8_t padding[3] = {0, 0, 0};
    pcap_write(data, datalen);
    if (datalen & 3)
        pcap_write(padding, 4-(datalen & 0x3));
}

static void pcap_write_u16(uint16_t value) {
     pcap_write(&value, sizeof(value));
}

static void pcap_write_u32(uint32_t value) {
    pcap_write(&value, sizeof(value));
}

static void pcap_write_u64(uint64_t value) {
    pcap_write(&value, sizeof(value));
}

static void pcap_write_option(pcap_option_t code, const void *data, size_t datalen) {
    pcap_write_u16(code);
    pcap_write_u16(datalen);
    if (datalen)
        pcap_write_padded(data, datalen);
}

static void pcap_write_u32_option(pcap_option_t code, uint32_t value) {
    pcap_write_option(code, &value, sizeof(value));
}

static void pcap_write_shb(void) {
    enum { SHB_LEN = 8 };
    pcap_write_u32(PCAP_MAGIC);
    pcap_write_u32(SHB_LEN*sizeof(uint32_t));
    pcap_write_u32(PCAP_BYTE_ORDER_MAGIC);
    pcap_write_u16(PCAP_MAJOR);
    pcap_write_u16(PCAP_MINOR);
    pcap_write_u64(~UINT64_C(0));
    pcap_write_option(PCAP_OPT_END, NULL, 0);
    pcap_write_u32(SHB_LEN*sizeof(uint32_t));
}

static void pcap_write_idb(void) {
    enum { IDB_LEN = 6 };
    pcap_write_u32(PCAP_TYPE_IDB);
    pcap_write_u32(IDB_LEN * sizeof(uint32_t));
    pcap_write_u16(LINKTYPE_AX25);
    pcap_write_u16(0 /* reserved */);
    pcap_write_u32(PCAP_NO_SNAPLEN);
    pcap_write_option(PCAP_OPT_END, NULL, 0);
    pcap_write_u32(IDB_LEN * sizeof(uint32_t));
}

static uint32_t calc_padding(uint32_t len) {
    if (len & 3)
        return 4 - (len & 3);
    return 0;
}

static uint32_t round_to_u32(uint32_t len) {
    return len + calc_padding(len);
}

/* 96 / 81 */
static void pcap_write_epb(capture_dir_t dir, const uint8_t data[], size_t datalen) {
    enum { EPB_BASE_LEN = 11 };
    pcap_write_u32(PCAP_TYPE_EPB);
    pcap_write_u32(EPB_BASE_LEN * sizeof(uint32_t) + round_to_u32(datalen));
    pcap_write_u32(0 /* Interface ID */);
    instant_t now = instant_now();
    duration_t unix = instant_sub(now, INSTANT_ZERO); /* Assumes INSTANT_ZERO is 1970-01-01 00:00:00 UTC */
    pcap_write_u32(duration_as_millis(unix) >> 32);
    pcap_write_u32(duration_as_millis(unix) & 0xFFFFFFFF);
    pcap_write_u32(datalen); /* captured len */
    pcap_write_u32(datalen); /* original len */
    pcap_write_padded(data, datalen);
    switch (dir) {
        case DIR_IN:
            pcap_write_u32_option(PCAP_OPT_FLAGS, PCAP_FLAG_IN | PCAP_FLAG_UNICAST);
            break;
        case DIR_OUT:
            pcap_write_u32_option(PCAP_OPT_FLAGS, PCAP_FLAG_OUT | PCAP_FLAG_UNICAST);
            break;
        case DIR_OTHER:
            pcap_write_u32_option(PCAP_OPT_FLAGS, PCAP_FLAG_IN | PCAP_FLAG_PROMISC);
            break;
    }
    pcap_write_option(PCAP_OPT_END, NULL, 0);
    pcap_write_u32(EPB_BASE_LEN * sizeof(uint32_t) + round_to_u32(datalen));
}

static capture_t pcap_capture = {
    .next = NULL,
    .capture = pcap_write_epb,
};

void pcap_init(void) {
    posix_fd = open("ax25.pcap", O_WRONLY | O_CREAT, 0666);
    if (posix_fd == -1)
        panic("Failed to open pcap file");

    pcap_write_shb();
    pcap_write_idb();
    capture_register(&pcap_capture);
}
