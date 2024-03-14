#include "connection.h"
#include "ssid.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

enum {
    ADDR_DST = 0,
    ADDR_SRC = 1,
    ADDR_DIGI1 = 2,
    ADDR_DIGI2 = 3,
};

typedef enum type_t {
    TYPE_PREV0= 0b00, /* Used in previous versions */
    TYPE_CMD  = 0b01, /* Command */
    TYPE_RES  = 0b10, /* Response */
    TYPE_PREV3= 0b11, /* Used in previous versions */
} type_t;

typedef enum ax25_dl_event_type_t {
    EV_CTRL_ERROR,
    EV_INFO_NOT_PERMITTED,
    EV_INCORRECT_LENGTH,
    /* Data link commands */
    EV_DL_DATA,
    EV_DL_UNIT_DATA,
    EV_DL_CONNECT_REQUEST,
    EV_DL_DISCONNECT,
    EV_DL_FLOW_ON,
    EV_DL_FLOW_OFF,
    /* Packets */
    EV_UA,
    EV_DM,
    EV_UI,
    EV_DISC,
    EV_SABM,
    EV_SABME,
    EV_TEST,
    EV_I,
    EV_RR,
    EV_RNR,
    EV_REJ,
    EV_SREJ,
    EV_FRMR,
    EV_UNKNOWN_FRAME,
    /* Timers */
    EV_TIMER_EXPIRE_T1,
    EV_TIMER_EXPIRE_T3,
} ax25_dl_event_type_t;

typedef struct ax25_dl_event_t {
    ax25_dl_event_type_t event;
    uint8_t port;
    size_t address_count;
    type_t type;
    uint8_t nr;
    uint8_t ns;
    uint8_t *info;
    uint8_t info_len;
    bool pf;
    ssid_t address[MAX_ADDRESSES];
    connection_t *conn;
} ax25_dl_event_t;

void ax25_dl_event(ax25_dl_event_t *ev);
