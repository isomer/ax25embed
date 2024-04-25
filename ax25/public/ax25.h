/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Definitions for AX.25 Framing layer
 */
#ifndef AX25_H
#define AX25_H 1
#include <stdint.h>
#include <stddef.h>

typedef enum pid_t {
    PID_PLP             = 0x01, /* ISO 8208/CCITT X.25 PLP */
    PID_VJCOMPIP        = 0x06, /* Compressed TCP/IP packet.  Van Jacobson (RFC 1144) */
    PID_VJIP            = 0x07, /* Uncompressed TCP/IP packet.  Van Jacobson (RFC 1144) */
    PID_SEGMENT         = 0x08, /* Segmentation fragment */
    PID_TEXNET          = 0xC3, /* TEXNET datagram protocol */
    PID_LINK_QUAL       = 0xC4, /* Link Quality Protocol */
    PID_APPLETALK       = 0xCA, /* Appletalk */
    PID_APPLETALK_ARP   = 0xCB, /* Appletalk ARP */
    PID_IP              = 0xCC, /* ARPA Internet Protocol */
    PID_ARP             = 0xCD, /* ARPA Address Resolution */
    PID_FLEXNET         = 0xCE, /* FlexNet */
    PID_NETROM          = 0xCF, /* NET/ROM */
    PID_NOL3            = 0xF0, /* No Layer 3 implemented */
    PID_ESCAPE          = 0xFF, /* Escape Character, next octet contains more L3 information */
} pid_t;

enum {
    FRAME_SABME = 0b01101111,
    FRAME_SABM  = 0b00101111,
    FRAME_DISC  = 0b01000011,
    FRAME_DM    = 0b00001111,
    FRAME_UA    = 0b01100011,
    FRAME_FRMR  = 0b10000111,
    FRAME_UI    = 0b00000011,
    FRAME_XID   = 0b10101111,
    FRAME_TEST  = 0b11100011,
    FRAME_RR    = 0b00000001,
    FRAME_RNR   = 0b00000101,
    FRAME_REJ   = 0b00001001,
    FRAME_SREJ  = 0b00001101,
    FRAME_I     = 0b00000001,

    FRAME_P     = 0b00010000,
    FRAME_F     = 0b00010000,

    FRAME16_P   = 0b0000000100000000,
    FRAME16_F   = 0b0000000100000000,

};

void ax25_recv(uint8_t port, const uint8_t pkt[], size_t pktlen);
void ax25_recv_ackmode(uint8_t port, uint16_t id, const uint8_t pkt[], size_t pktlen);

#endif
