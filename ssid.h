#ifndef SSID_H
#define SSID_H 1
#include "packet.h"
#include <stdint.h>
#include <stdbool.h>

enum {
    SSID_LEN = 7,
    MAX_ADDRESSES = 4,
};

typedef struct ssid_t {
    char ssid[SSID_LEN];
} ssid_t;

bool ssid_parse(const uint8_t buffer[static SSID_LEN], ssid_t *ssid);
void ssid_debug(const ssid_t *ssid);
bool ssid_push(packet_t *packet, const ssid_t *ssid);
bool ssid_cmp(const ssid_t *lhs, const ssid_t *rhs);
bool ssid_is_mine(const ssid_t *ssid);

#endif
