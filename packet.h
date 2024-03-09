#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum {
    MAX_PACKET_SIZE = 384,
};

typedef struct packet_t {
    bool in_use;
    uint8_t port;
    size_t len;
    struct packet_t *next;
    uint8_t buffer[MAX_PACKET_SIZE];
} packet_t;

packet_t *packet_allocate(void);
void packet_free(packet_t **packet);
void packet_push(packet_t *packet, const void *ptr, size_t ptrlen);
void packet_push_byte(packet_t *packet, uint8_t byte);

