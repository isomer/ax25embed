#include "ax25.h"
#include "ssid.h"
#include "packet.h"
#include <stdlib.h>

ssid_t dst;
ssid_t src;

static void fuzz_test(const uint8_t *data, size_t len) {
    packet_t *pkt = packet_allocate();
    ssid_push(pkt, &dst);
    ssid_push(pkt, &src);
    /* add end of addresses marker */
    pkt->buffer[pkt->len-1] |= 0b00000001;
    pkt->buffer[SSID_LEN-1]   |= (data[len-1] & 1) != 0 ? 0b10000000 : 0;
    pkt->buffer[2*SSID_LEN-1] |= (data[len-1] & 2) != 0 ? 0b10000000 : 0;
    packet_push(pkt, data, len-1);
    ax25_recv(0, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (!ssid_from_string("M7QQQ-0", &dst))
        abort();
    if (!ssid_from_string("M7QQQ-1", &src))
        abort();
    ssid_set_local(&dst);
    int count = 0;
    while (size > 0) {
        size_t len = data[0] + 1;
        if (len > size)
            break;
        fuzz_test(data, len);
        data += len;
        size -= len;
        ++count;
    }
    return 0;
}
