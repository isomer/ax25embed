#include "packet.h"
#include "metric.h"
#include "platform.h"
#include <string.h> // for memcpy

enum {
    MAX_PACKETS = 8,
};

static packet_t packets[MAX_PACKETS];
static size_t packet_next = 0;
static size_t in_use = 0;

packet_t *packet_allocate(void) {
    if (in_use >= MAX_PACKETS) {
        metric_inc(METRIC_NO_PACKETS);
        return NULL;
    }

    while (packets[packet_next].in_use)
        packet_next++;

    CHECK(!packets[packet_next].in_use);

    packets[packet_next].in_use = true;
    packets[packet_next].port = 255;
    packets[packet_next].next = NULL;
    packets[packet_next].len = 0;
    metric_inc(METRIC_PACKETS_ALLOCATED);

    in_use++;

    return &packets[packet_next++];
}

void packet_free(packet_t **packet) {
    CHECK((*packet)->in_use);
    (*packet)->in_use = false;
    CHECK(in_use > 0);
    in_use--;
    *packet = NULL;
    metric_inc(METRIC_PACKETS_FREED);
}

void packet_push(packet_t *packet, const void *ptr, size_t ptrlen) {
    CHECK(ptrlen + packet->len < MAX_PACKET_SIZE);
    memcpy(&packet->buffer[packet->len], ptr, ptrlen);
    packet->len += ptrlen;
}

void packet_push_byte(packet_t *packet, uint8_t byte) {
    CHECK(packet->len + 1< MAX_PACKET_SIZE);
    packet->buffer[packet->len++] = byte;
}
