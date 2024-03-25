/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Based on AX.25 v2.2 (https://www.tapr.org/pdf/AX25.2.2.pdf)
 *
 * This is currently as literal translation of the SDL as I can possibly make it.
 *
 * Notes:
 *  - TEST frames aren't handled anywhere?
 *  - The SDL talks about "TIV" and "Next T1 value" but they are both actually "T1V"
 *  - The SDL says "last T1 value * 2 ** (RC+1) * SRTT" but it actually means "T1V = 2 ** (RC+1) * SRTT"
 *  - The SDL says "set version 2.0" and "set version 2.2" but those should be shown as function calls.
 *  - Why do the "set version" functions set T2 to 3s?
 *  - Figure C4.1 is blank, and would be really useful.
 *  - In section C4.3, it lists states 0..4, then immediately lists errors for
 *    state 5 (and defines state 5 in the SDL)
 *  - Section C4.3 is missing many fields that are referred to in the SDL (eg RC retry count).
 *  - set version * procedures set "n1r" which is never referred to anywhere else, presumably it's just N1.
 *  - "The Retries parameter field (PI=10) allows the negotiation of the retry count (N1)." should be N2.
 *  - DL ERROR indication (K) is not defined, but are used in multiple different errors?
 *  - DL ERROR indication (G) is not defined.  It is "connection timed out"
 *  - DL ERROR indication (H) is not defined.  It is "connection timed out while disconnecting"
 *  - SABE on page 99 should be SABME
 *  - page 99: second V(s) <-- 0 should be V(r) <-- 0.
 *  - dicard should be discard.
 *  - page 100: "cleak Ackonwledge Pending"
 *  - page 100: "N9r)" should be "N(r)"
 *  - FRMR on page 101 refers to "Note 1" but no such note seems to exist anywhere.
 *  - Timer 1 expiry in "Timer Recovery" state does not stop the timer before disconnecting.
 */
#include "ax25_dl.h"
#include "ax25.h"
#include "config.h"
#include "buffer.h"
#include "kiss.h"
#include "platform.h"

static dl_socket_t dl_sockets[MAX_SOCKETS];

dl_socket_t listen_socket = {
    .conn = NULL,
    .userdata = NULL,
    .on_connect = NULL,
    .on_error = NULL,
    .on_data = NULL,
    .on_disconnect = NULL,
};

static dl_socket_t *socket_allocate(connection_t *conn) {
    for(size_t i = 0; i < MAX_SOCKETS; ++i) {
        if (dl_sockets[i].conn == NULL) {
            dl_sockets[i].conn = conn;
            conn->socket = &dl_sockets[i];
            return &dl_sockets[i];
         }
    }
    return NULL;
}

static void socket_free(dl_socket_t *socket) {
    socket->conn->socket = NULL;
    socket->conn = NULL;
    socket = NULL;
}

static void push_reply_addrs(ax25_dl_event_t *ev, packet_t *pkt, type_t type) {
    pkt->port = ev->conn ? ev->conn->port : ev->port;
    if (ev->address_count) {
        ssid_push(pkt, &ev->address[ADDR_SRC]);
        ssid_push(pkt, &ev->address[ADDR_DST]);

        /* Add digipeater addresses in reverse order */
        for(size_t i=ev->address_count-1; i>=ADDR_DIGI1; i--) {
            ssid_push(pkt, &ev->address[i]);
        }
    } else {
        ssid_push(pkt, &ev->conn->remote);
        ssid_push(pkt, &ev->conn->local);
        /* TODO: no digipeaters? */
    }

    /* add end of addresses marker */
    pkt->buffer[pkt->len-1] |= 0b00000001;

    pkt->buffer[SSID_LEN] |= (type & 0b01) != 0 ?  0b10000000 : 0;
    pkt->buffer[2*SSID_LEN] |= (type & 0b10) != 0 ? 0b10000000 : 0;
}

static void push_u_control(packet_t *pkt, uint8_t cmd, type_t type, bool p, bool f) {
    /* TODO: support MODULO_128 */
    if (type == TYPE_RES) {
        cmd |= (f ? FRAME_F : 0);
    } else {
        cmd |= (p ? FRAME_P : 0);
    }
    packet_push_byte(pkt, cmd);
}

static void push_s_control(packet_t *pkt, uint8_t cmd, type_t type, bool p, bool f, uint8_t nr) {
    /* TODO: support MODULO_128 */
    if (type == TYPE_RES) {
        cmd |= (f ? FRAME_F : 0);
    } else {
        cmd |= (p ? FRAME_P : 0);
    }
    cmd |= (nr << 5) & 0b11100000;
    packet_push_byte(pkt, cmd);
}

static void push_i_control(packet_t *pkt, bool p, uint8_t nr, uint8_t ns) {
    /* TODO: support MODULO_128 */
    uint8_t ctl = 0;
    ctl |= (p ? FRAME_P : 0);
    ctl |= (nr << 5) & 0b11100000;
    ctl |= (ns << 1) & 0b00001110;
    packet_push_byte(pkt, ctl);
}

static void send_dm(ax25_dl_event_t *ev, bool f, bool expedited) {
    (void) expedited;
    DEBUG(STR("sending dm"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, TYPE_RES);
    push_u_control(pkt, FRAME_DM, TYPE_RES, ev->p, f);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_ui(ax25_dl_event_t *ev, type_t type) {
    DEBUG(STR("sending ui"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, type);
    push_u_control(pkt, FRAME_UI, type, ev->p, ev->f);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_ua(ax25_dl_event_t *ev, bool expedited) {
    (void) expedited;
    DEBUG(STR("sending ua"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, TYPE_RES);
    push_u_control(pkt, FRAME_UA, TYPE_RES, ev->p, ev->f);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_sabm(ax25_dl_event_t *ev, bool f) {
    DEBUG(STR("sending sabm"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, TYPE_CMD);
    push_u_control(pkt, FRAME_SABM, TYPE_CMD, ev->p, f);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_sabme(ax25_dl_event_t *ev, bool f) {
    DEBUG(STR("sending sabme"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, TYPE_CMD);
    push_u_control(pkt, FRAME_SABME, TYPE_CMD, ev->p, f);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_disc(ax25_dl_event_t *ev, bool f) {
    DEBUG(STR("sending disc"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, TYPE_CMD);
    push_u_control(pkt, FRAME_DISC, TYPE_CMD, ev->p, f);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_test(ax25_dl_event_t *ev, type_t type, bool f) {
    DEBUG(STR("sending test"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, type);
    push_u_control(pkt, FRAME_TEST, type, ev->p, f);
    packet_push(pkt, ev->info, ev->info_len);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_srej(ax25_dl_event_t *ev, type_t type) {
    DEBUG(STR("sending srej"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, type);
    push_s_control(pkt, FRAME_SREJ, type, ev->p, ev->f, ev->conn->rcv_state);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_rej(ax25_dl_event_t *ev, type_t type) {
    DEBUG(STR("sending rej"));
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, type);
    push_s_control(pkt, FRAME_REJ, TYPE_RES, ev->p, ev->f, ev->conn->rcv_state);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_rr(ax25_dl_event_t *ev, type_t type, bool f) {
    DEBUG(STR("sending rr"));

    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, type);
    push_s_control(pkt, FRAME_RR, type, ev->p, f, ev->conn->rcv_state);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static void send_rnr(ax25_dl_event_t *ev, type_t type, bool f) {
    DEBUG(STR("sending rnr"));

    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, type);
    push_s_control(pkt, FRAME_RNR, type, ev->p, f, ev->conn->rcv_state);

    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
    packet_free(&pkt);
}

static packet_t *construct_i(ax25_dl_event_t *ev, uint8_t *info, size_t info_len, uint8_t nr) {
    packet_t *pkt = packet_allocate();

    push_reply_addrs(ev, pkt, TYPE_RES);
    push_i_control(pkt, ev->p, nr, ev->conn->snd_state);
    packet_push(pkt, info, info_len);

    return pkt;
}

static void dl_error(ax25_dl_event_t *ev, ax25_dl_error_t err) {
    if (ev->conn->socket->on_error)
        ev->conn->socket->on_error(ev->conn->socket, err);
}

static void dl_data_indication(ax25_dl_event_t *ev, const uint8_t *data, size_t datalen) {
    if (ev->conn->socket->on_data)
        ev->conn->socket->on_data(ev->conn->socket, data, datalen);
}

static void dl_unit_data_indication(ax25_dl_event_t *ev, const uint8_t *data, size_t datalen) {
    (void) ev;
    /* unconnected data appears on the listen socket */
    listen_socket.on_data(&listen_socket, data, datalen);
}

static void dl_connect_indication(ax25_dl_event_t *ev) {
    if (ev->conn->socket->on_connect)
        ev->conn->socket->on_connect(ev->conn->socket);
}

static void dl_disconnect_indication(ax25_dl_event_t *ev) {
    if (ev->conn && ev->conn->socket->on_disconnect)
        ev->conn->socket->on_disconnect(ev->conn->socket);
}

dl_socket_t *dl_connect(ssid_t *remote, ssid_t *local, uint8_t port) {
    ax25_dl_event_t ev;
    ev.type = TYPE_CMD;
    ev.event = EV_DL_CONNECT;
    ev.address_count = 2;
    ev.address[ADDR_DST] = *local;
    ev.address[ADDR_SRC] = *remote;
    ev.port = port;
    ev.conn = NULL;
    ax25_dl_event(&ev);
    return ev.conn ? socket_allocate(ev.conn) : NULL;
}

void dl_send(dl_socket_t *sock, const void *data, size_t datalen) {
    ax25_dl_event_t ev;
    ev.event = EV_DL_DATA;
    ev.conn = sock->conn;
    ev.info = data;
    ev.info_len = datalen;

    ax25_dl_event(&ev);
}

static buffer_t *pop_queue(connection_t *conn) {
    if (!conn->send_queue_head)
        return NULL;

    buffer_t *ret = conn->send_queue_head;
    conn->send_queue_head = conn->send_queue_head->next;
    if (!conn->send_queue_head) {
        conn->send_queue_tail = NULL;
    }

    return ret;
}

static void discard_queue(connection_t *conn) {
    while (conn->send_queue_head) {
        buffer_t *buf = pop_queue(conn);
        buffer_free(buf);
    }
}

static void push_i(connection_t *conn, buffer_t *buffer) {
    CHECK(!buffer->next);

    if (conn->send_queue_tail) {
        CHECK(conn->send_queue_head);
        conn->send_queue_tail->next = buffer;
        conn->send_queue_tail = buffer;
    } else {
        CHECK(!conn->send_queue_head);
        conn->send_queue_tail = buffer;
        conn->send_queue_head = buffer;
    }
}

static void push_old_i_frame_nr_on_queue(ax25_dl_event_t *ev) {
    packet_t *pkt = ev->conn->sent_buffer[ev->nr];
    kiss_xmit(pkt->port, pkt->buffer, pkt->len);
}

static void set_state(connection_t *conn, conn_state_t state) {
    conn->state = state;
    if (state == STATE_DISCONNECTED) {
        if (conn->socket)
            socket_free(conn->socket);
        conn_release(conn);
    }
}

static bool timer_running_t1(ax25_dl_event_t *ev) {
    return (instant_cmp(ev->conn->t1_expiry, INSTANT_ZERO) != 0);
}

static void timer_start_t1(ax25_dl_event_t *ev) {
    ev->conn->t1_expiry = instant_add(instant_now(), ev->conn->t1v);
}

static void timer_stop_t1(ax25_dl_event_t *ev) {
    ev->conn->t1_remaining = instant_sub(ev->conn->t1_expiry, instant_now());
    if (duration_cmp(ev->conn->t1_remaining, DURATION_ZERO) < 0) {
        ev->conn->t1_remaining = DURATION_ZERO;
    }
    ev->conn->t1_expiry = INSTANT_ZERO;
}

static bool timer_expired_t1(ax25_dl_event_t *ev) {
    return instant_cmp(ev->conn->t1_expiry, instant_now()) > 0;
}

static void timer_start_t2(ax25_dl_event_t *ev) {
    ev->conn->t2_expiry = instant_add(instant_now(), ev->conn->t2);
}

static void timer_stop_t2(ax25_dl_event_t *ev) {
    ev->conn->t2_expiry = INSTANT_ZERO;
}

static void timer_start_t3(ax25_dl_event_t *ev) {
    ev->conn->t3_expiry = instant_add(instant_now(), duration_minutes(T3_DURATION_MINUTES));
}

static void timer_stop_t3(ax25_dl_event_t *ev) {
    ev->conn->t3_expiry = INSTANT_ZERO;
}

static void clear_exception_conditions(ax25_dl_event_t *ev) {
    ev->conn->peer_busy = false;
    ev->conn->rej_exception = false;
    ev->conn->self_busy = false;
    ev->conn->ack_pending = false;
}

static void establish_data_link(ax25_dl_event_t *ev) {
    clear_exception_conditions(ev);
    ev->conn->rc = 0;
    ev->p = true;
    send_sabm(ev, ev->p);
    timer_stop_t3(ev);
    timer_start_t3(ev);
}

static void nr_error_recovery(ax25_dl_event_t *ev) {
    dl_error(ev, ERR_J);
    establish_data_link(ev);
    ev->conn->l3_initiated = false;
}


static void transmit_inquiry(ax25_dl_event_t *ev) {
    ev->p = true;
    ev->nr = ev->conn->rcv_state;
    if (ev->conn->self_busy) {
        send_rnr(ev, TYPE_CMD, ev->f);
    } else {
        send_rr(ev, TYPE_CMD, ev->f);
    }
    ev->conn->ack_pending = false;
    timer_start_t1(ev);
    timer_stop_t2(ev);
}

static void enquiry_response(ax25_dl_event_t *ev, bool f) {
    ev->nr = ev->conn->rcv_state;
    if (ev->conn->self_busy) {
        send_rnr(ev, TYPE_RES, f);
    } else {
        send_rr(ev, TYPE_RES, f);
    }
    ev->conn->ack_pending = false;
    timer_stop_t2(ev);
}

static void invoke_retransmission(ax25_dl_event_t *ev) {
    /* backtrack */
    uint8_t x = ev->conn->snd_state;
    ev->conn->snd_state = ev->nr;
    do {
        push_old_i_frame_nr_on_queue(ev);
        ev->conn->snd_state += 1;
    } while (ev->conn->snd_state != x);
}

static void select_t1(ax25_dl_event_t *ev) {
    if (ev->conn->rc) {
        duration_t srtt = duration_mul(ev->conn->srtt, 7);
        srtt = duration_add(srtt, ev->conn->t1v);
        srtt = duration_sub(srtt, ev->conn->t1_remaining);
        srtt = duration_div(srtt, 8);
        ev->conn->srtt = srtt;
        ev->conn->t1v = duration_mul(ev->conn->srtt, 2);
    } else if (timer_expired_t1(ev)) {
          ev->conn->t1v = duration_mul(ev->conn->srtt, (1 << (ev->conn->rc+1)));
    }
}

static void check_i_frame_acked(ax25_dl_event_t *ev) {
    if (ev->conn->peer_busy) {
        ev->conn->ack_state = ev->nr;
        if (!timer_running_t1(ev)) {
            timer_start_t1(ev);
        }
    } else if (ev->nr == ev->conn->snd_state) {
        ev->conn->ack_state = ev->nr;
        timer_stop_t1(ev);
        timer_stop_t2(ev);
        timer_stop_t3(ev);
        select_t1(ev);
    } else if (ev->nr != ev->conn->ack_state) {
        ev->conn->ack_state = ev->nr;
        timer_start_t1(ev);
    }
}

static void check_need_for_response(ax25_dl_event_t *ev) {
    if (ev->type == TYPE_CMD) {
        enquiry_response(ev, /* f= */ true);
    } else {
        if (ev->type == TYPE_RES && ev->f) {
            dl_error(ev, ERR_A);
        }
    }
}

static void ui_check(ax25_dl_event_t *ev) {
    if (ev->type == TYPE_CMD) {
        if (ev->info_len < ev->conn->n1) {
            dl_unit_data_indication(ev, ev->info, ev->info_len);
        } else {
            dl_error(ev, ERR_N); /* Defined as being error K, but it's not defined what it is! */
        }
    } else {
        dl_error(ev, ERR_Q);
    }
}

static void establish_extended_data_link(ax25_dl_event_t *ev) {
    clear_exception_conditions(ev);
    ev->conn->rc = 0;
    bool p = true;
    send_sabme(ev, p);
    timer_stop_t3(ev);
    timer_start_t3(ev);
}

static void set_version_2_0(ax25_dl_event_t *ev) {
    ev->conn->srej_enabled = false;
    ev->conn->modulo = 8;
    ev->conn->n1 = 2048;
    ev->conn->window_size = 4;
    ev->conn->t2 = duration_seconds(3);
    ev->conn->n2 = 10;
}

static void set_version_2_2(ax25_dl_event_t *ev) {
    ev->conn->srej_enabled = false;
    ev->conn->modulo = 128;
    ev->conn->n1 = 2048;
    ev->conn->window_size = 32;
    ev->conn->t2 = duration_seconds(3);
    ev->conn->n2 = 10;
}

static void ax25_dl_disconnected(ax25_dl_event_t *ev) {
    switch (ev->event) {
        case EV_CTRL_ERROR:
            dl_error(ev, ERR_L);
            break;
        case EV_INFO_NOT_PERMITTED:
            dl_error(ev, ERR_M);
            break;
        case EV_INCORRECT_LENGTH:
            dl_error(ev, ERR_N);
            break;
        case EV_UA:
            dl_error(ev, ERR_C);
            dl_error(ev, ERR_D);
            break;

        case EV_DM:
            /* set state to disconnected */
            break;

        case EV_UI:
            ui_check(ev);
            if (ev->p) {
                send_dm(ev, /* f= */ true, /* expedited= */ false);
            }
            break;

        case EV_TEST:
            if (ev->type == TYPE_CMD) {
                send_test(ev, TYPE_RES, ev->f);
            }
            break;

         case EV_DL_DISCONNECT:
            /* confirm */
            break;

         case EV_DISC:
            ev->f = ev->p;
            send_dm(ev, ev->f, /* expedited=*/ false);
            break;

         case EV_DL_UNIT_DATA:
            send_ui(ev, TYPE_CMD);
            break;

         /* All other commands */
         case EV_UNKNOWN_FRAME:
         case EV_I:
         case EV_RR:
         case EV_RNR:
         case EV_REJ:
         case EV_SREJ:
         case EV_FRMR:
            ev->f = ev->p;
            send_dm(ev, ev->f, /* expedited= */ false);
            break;

         /* All other primatives */
         case EV_DL_DATA:
         case EV_DL_FLOW_ON:
         case EV_DL_FLOW_OFF:
         case EV_TIMER_EXPIRE_T1:
         case EV_TIMER_EXPIRE_T3:
         case EV_LM_DATA:
         case EV_TIMER_EXPIRE_T2:
            break;

         case EV_DL_CONNECT:
            ev->conn = conn_find_or_create(&ev->address[ADDR_DST], &ev->address[ADDR_SRC], ev->port);
            ev->conn->srtt = duration_millis(200);
            ev->conn->t1v = duration_mul(ev->conn->srtt, 2);

            dl_socket_t *sock = socket_allocate(ev->conn);
            sock->on_connect = listen_socket.on_connect;

            establish_data_link(ev);

            ev->conn->l3_initiated = true;

            set_state(ev->conn, STATE_AWAITING_CONNECTION);

            break;

         case EV_SABM:
         case EV_SABME:
            DEBUG(STR("recv SABM(E?)"));
            ev->f = ev->p;

            ev->conn = conn_find_or_create(&ev->address[ADDR_DST], &ev->address[ADDR_SRC], ev->port);
            if (!ev->conn) {
                send_dm(ev, ev->f, /* expedited= */ false);
            } else {
                send_ua(ev, false);
                ev->conn->snd_state = 0;
                ev->conn->ack_state = 0;
                ev->conn->rcv_state = 0;

                dl_socket_t *sock = socket_allocate(ev->conn);
                sock->on_connect = listen_socket.on_connect;
                dl_connect_indication(ev);

                ev->conn->srtt = duration_millis(200);
                ev->conn->t1v = duration_mul(ev->conn->srtt, 2);

                set_state(ev->conn, STATE_CONNECTED);
                ev->conn->l3_initiated = false; /* TODO: Check what this should be set to? */
                timer_start_t3(ev);
            }
            if (ev->event == EV_SABM) {
                set_version_2_0(ev);
            } else {
                set_version_2_2(ev);
            }
            break;
    }
}

static void ax25_dl_awaiting_connection(ax25_dl_event_t *ev) {
    switch(ev->event) {
        case EV_CTRL_ERROR:
            dl_error(ev, ERR_L);
            break;
        case EV_INFO_NOT_PERMITTED:
            dl_error(ev, ERR_M);
            break;
        case EV_INCORRECT_LENGTH:
            dl_error(ev, ERR_N);
            break;
        case EV_DL_CONNECT:
            discard_queue(ev->conn);
            ev->conn->l3_initiated = true;
            break;
        case EV_DL_DISCONNECT:
            /* TODO: requeue */
            break;
        case EV_SABM:
            ev->f = ev->p;
            send_ua(ev, false);
            break;
        case EV_DISC:
            ev->f = ev->p;
            send_dm(ev, ev->f, /* expedited= */ false);
            break;
        case EV_DL_DATA:
            if (!ev->conn->l3_initiated) {
                buffer_t *buf = buffer_allocate(ev->info, ev->info_len);
                push_i(ev->conn, buf);
            }
            break;

        case EV_DRAIN_SENDQ:
            if (!ev->conn->l3_initiated) {
                /* Don't dequeue frame */
            } else {
                buffer_t *buffer = pop_queue(ev->conn);
                buffer_free(buffer);
            }
            break;

        case EV_UI:
            ui_check(ev);
            if (ev->p) {
                send_dm(ev, true, /* expedited= */ false);
            }
            break;

        case EV_TEST:
            if (ev->type == TYPE_CMD) {
                send_test(ev, TYPE_RES, ev->f);
            }
            break;

        case EV_DL_UNIT_DATA:
            send_ui(ev, TYPE_CMD);
            break;

         /* All other primitives */
         case EV_TIMER_EXPIRE_T3:
         case EV_DL_FLOW_OFF:
         case EV_DL_FLOW_ON:
         case EV_UNKNOWN_FRAME:
         case EV_I:
         case EV_RR:
         case EV_RNR:
         case EV_REJ:
         case EV_SREJ:
         case EV_FRMR:
         case EV_LM_DATA:
         case EV_TIMER_EXPIRE_T2:
            break;

         case EV_DM:
            if (ev->f) {
                discard_queue(ev->conn);
                dl_disconnect_indication(ev);
                timer_stop_t1(ev);

                set_state(ev->conn, STATE_DISCONNECTED);
            } else {
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
            }
            break;
         case EV_UA:
            if (ev->f) {
                if (ev->conn->l3_initiated) {
                    dl_disconnect_indication(ev);
                } else {
                    if (ev->conn->snd_state != ev->conn->ack_state) {
                        discard_queue(ev->conn);
                        dl_disconnect_indication(ev);
                    }
                    timer_stop_t1(ev);
                    timer_stop_t2(ev);
                    timer_stop_t3(ev);
                    ev->conn->snd_state = ev->conn->ack_state = ev->conn->rcv_state = 0;
                    select_t1(ev);
                    set_state(ev->conn, STATE_CONNECTED);
                }
            } else {
                dl_error(ev, ERR_D);
            }
            break;

         case EV_TIMER_EXPIRE_T1:
            if (ev->conn->rc == ev->conn->n2) {
                discard_queue(ev->conn);
                dl_error(ev, ERR_G);
                dl_disconnect_indication(ev);
                set_state(ev->conn, STATE_DISCONNECTED);
            } else {
                ev->conn->rc = ev->conn->rc + 1;
                send_sabm(ev, /* p=*/ true);
                select_t1(ev);
                timer_start_t1(ev);
            }
            break;

         case EV_SABME:
            ev->f = ev->p;
            send_dm(ev, ev->f, /* expedited= */ false);
            set_state(ev->conn, STATE_AWAITING_CONNECT_2_2);
            break;
    }
}

static void ax25_dl_awaiting_release(ax25_dl_event_t *ev) {
    switch(ev->event) {
        case EV_CTRL_ERROR:
            dl_error(ev, ERR_L);
            break;
        case EV_INFO_NOT_PERMITTED:
            dl_error(ev, ERR_M);
            break;
        case EV_INCORRECT_LENGTH:
            dl_error(ev, ERR_N);
            break;
        case EV_DL_DISCONNECT:
            send_dm(ev, false, /* expedited= */ true);
            timer_stop_t1(ev);
            timer_stop_t2(ev);
            set_state(ev->conn, STATE_DISCONNECTED);
            break;
        case EV_SABM:
            ev->f = ev->p;
            send_dm(ev, ev->f, /* expedited= */ true);
            break;

        case EV_DISC:
            ev->f = ev->p;
            send_ua(ev, /* expedited= */ true);
            break;

        case EV_DL_UNIT_DATA:
            send_ui(ev, TYPE_CMD);
            break;

        case EV_I:
        case EV_RR:
        case EV_RNR:
        case EV_REJ:
        case EV_SREJ:
            if (ev->p) {
                send_dm(ev, /* f= */ true, /* expedited= */ false);
            }
            break;

        case EV_UI:
            ui_check(ev);
            if (ev->p) {
                send_dm(ev, /* f= */ true, /* expedited= */ false);
            }
            break;

        case EV_TEST:
            if (ev->type == TYPE_CMD) {
                send_test(ev, TYPE_RES, ev->f);
            }
            break;

        /* all other primatives */
        case EV_TIMER_EXPIRE_T3:
        case EV_DL_FLOW_ON:
        case EV_DL_FLOW_OFF:
        case EV_UNKNOWN_FRAME:
        case EV_DL_CONNECT:
        case EV_DL_DATA:
        case EV_SABME:
        case EV_FRMR:
        case EV_LM_DATA:
        case EV_TIMER_EXPIRE_T2:
            break;

        case EV_UA:
            if (ev->f) {
                dl_disconnect_indication(ev);
                timer_stop_t1(ev);
                set_state(ev->conn, STATE_DISCONNECTED);
            } else {
                dl_error(ev, ERR_D);
            }
            break;

        case EV_DM:
            if (ev->f) {
                dl_connect_indication(ev); /* TODO: really?  Sure this isn't a disconnect notification? */
                timer_stop_t1(ev);
                set_state(ev->conn, STATE_DISCONNECTED);
            }
            break;

        case EV_TIMER_EXPIRE_T1:
            if (ev->conn->rc == ev->conn->n2) {
                dl_error(ev, ERR_H);
                dl_disconnect_indication(ev);
                set_state(ev->conn, STATE_DISCONNECTED);
            } else {
                ev->conn->rc = ev->conn->rc + 1;
                send_disc(ev, /* p= */ true);
                select_t1(ev);
                timer_start_t1(ev);
            }
    }
}

static void ax25_dl_connected(ax25_dl_event_t *ev) {
    switch (ev->event) {
        case EV_CTRL_ERROR:
            dl_error(ev, ERR_L);
            discard_queue(ev->conn);
            establish_data_link(ev);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_INFO_NOT_PERMITTED:
            dl_error(ev, ERR_M);
            discard_queue(ev->conn);
            establish_data_link(ev);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_INCORRECT_LENGTH:
            dl_error(ev, ERR_N);
            discard_queue(ev->conn);
            establish_data_link(ev);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_DL_CONNECT:
            discard_queue(ev->conn);
            establish_data_link(ev);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_DL_DISCONNECT:
            discard_queue(ev->conn);
            ev->conn->rc = 0;
            send_disc(ev, /* p= */ true);
            timer_stop_t3(ev);
            timer_start_t1(ev);
            set_state(ev->conn, STATE_AWAITING_RELEASE);
            break;

        case EV_DL_DATA: {
            buffer_t *buf = buffer_allocate(ev->info, ev->info_len);
            push_i(ev->conn, buf);
            break;
         }

        case EV_DRAIN_SENDQ:
            if (ev->conn->peer_busy
                    || ev->conn->snd_state == ev->conn->ack_state + ev->conn->window_size ) {
                /* Leave sendq buffer on queue */
            } else {
                ev->ns = ev->conn->snd_state;
                ev->nr = ev->conn->rcv_state;
                ev->p = false;

                buffer_t *buf = pop_queue(ev->conn);
                packet_t *pkt = construct_i(ev, buf->buffer, buf->len, ev->nr);
                kiss_xmit(ev->conn->port, pkt->buffer, pkt->len);
                buffer_free(buf);
                if (ev->conn->sent_buffer[ev->ns]) {
                    packet_free(&ev->conn->sent_buffer[ev->ns]);
                }
                ev->conn->sent_buffer[ev->ns] = pkt;

                ev->conn->snd_state = (ev->conn->snd_state + 1) % ev->conn->modulo;
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                if (timer_running_t1(ev)) {
                    timer_stop_t3(ev);
                    timer_start_t1(ev);
                }
            }
            break;

       case EV_TIMER_EXPIRE_T1:
            ev->conn->rc = 1;
            transmit_inquiry(ev);
            set_state(ev->conn, STATE_TIMER_RECOVERY);
            break;

       case EV_TIMER_EXPIRE_T3:
            ev->conn->rc = 0;
            transmit_inquiry(ev);
            set_state(ev->conn, STATE_TIMER_RECOVERY);
            break;

       case EV_SABM:
       case EV_SABME:
            ev->f = ev->p;
            send_ua(ev, false);
            clear_exception_conditions(ev);
            dl_error(ev, ERR_F);
            if (ev->conn->snd_state != ev->conn->ack_state) {
                discard_queue(ev->conn);
                dl_connect_indication(ev);
            }
            timer_stop_t1(ev);
            timer_start_t3(ev);
            ev->conn->snd_state = ev->conn->ack_state = ev->conn->rcv_state = 0;
            break;

       case EV_DISC:
            discard_queue(ev->conn);
            ev->f = ev->p;
            send_ua(ev, /* TODO: check */ false);
            dl_disconnect_indication(ev);
            timer_stop_t1(ev);
            timer_stop_t3(ev);
            set_state(ev->conn, STATE_DISCONNECTED);
            break;

       case EV_UA:
            dl_error(ev, ERR_C);
            establish_data_link(ev);
            ev->conn->l3_initiated = false;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

       case EV_DM:
            dl_error(ev, ERR_E);
            dl_disconnect_indication(ev);
            discard_queue(ev->conn);
            timer_stop_t1(ev);
            timer_stop_t3(ev);
            set_state(ev->conn, STATE_DISCONNECTED);
            break;

       case EV_FRMR:
            dl_error(ev, ERR_K); /* K is not defined */
            establish_data_link(ev);
            ev->conn->l3_initiated = false;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

       case EV_DL_FLOW_OFF:
            if (!ev->conn->self_busy) {
                ev->conn->self_busy = true;
                send_rnr(ev, TYPE_CMD, /* f= */ false);
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
            }
            break;

       case EV_DL_FLOW_ON:
            if (ev->conn->self_busy) {
                ev->conn->self_busy = false;
                send_rr(ev, TYPE_CMD, /* p= */ true);
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                if (!timer_running_t1(ev)) {
                    timer_stop_t3(ev);
                    timer_start_t1(ev);
                }
            }
            break;

       case EV_DL_UNIT_DATA:
            send_ui(ev, TYPE_CMD);
            break;

       case EV_UI:
            ui_check(ev);
            if (ev->p) {
                enquiry_response(ev, /* f= */ true);
            }
            break;

        case EV_TEST:
            if (ev->type == TYPE_CMD) {
                send_test(ev, TYPE_RES, ev->f);
            }
            break;

       case EV_RR:
            ev->conn->peer_busy = false;
            check_need_for_response(ev);
            if (seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                check_i_frame_acked(ev);
            } else {
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
            }
            break;

       case EV_RNR:
            ev->conn->peer_busy = true;
            check_need_for_response(ev);
            if (seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                check_i_frame_acked(ev);
            } else {
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
            }
            break;

       case EV_TIMER_EXPIRE_T2:
            if (ev->conn->ack_pending) {
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                enquiry_response(ev, /* f= */ false);
            }
            timer_stop_t2(ev);
            break;

       case EV_SREJ:
            ev->conn->peer_busy = false;
            check_need_for_response(ev);
            if (seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                if (ev->type == TYPE_CMD ? ev->p : ev->f) {
                    ev->conn->ack_state = ev->nr;
                }
                timer_stop_t1(ev);
                timer_start_t3(ev);
                select_t1(ev);
                push_old_i_frame_nr_on_queue(ev);
            } else {
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
            }
            break;

        case EV_REJ:
            ev->conn->peer_busy = false;
            check_need_for_response(ev);
            if (seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                ev->conn->ack_state = ev->nr;
                timer_stop_t1(ev);
                timer_stop_t3(ev);
                select_t1(ev);
                invoke_retransmission(ev);
            } else {
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
            }
            break;

        case EV_I:
            if (ev->type != TYPE_CMD) {
                dl_error(ev, ERR_S);
                break;
            }

            if (ev->info_len >= ev->conn->n1) {
                dl_error(ev, ERR_O);
                establish_data_link(ev);
                ev->conn->l3_initiated = false;
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
                break;
            }

            if (!seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                /* recieved ack out of window */
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
                break;
            }

            check_i_frame_acked(ev);

            if (ev->conn->self_busy) {
                /* discard contents of i frame */
                if (ev->p) {
                    ev->f = 1;
                    ev->nr = ev->conn->rcv_state;
                    send_rnr(ev, TYPE_RES, ev->f);
                    ev->conn->ack_pending = false;
                    timer_stop_t2(ev);
                }
                break;
            }

            if (ev->ns == ev->conn->rcv_state) {
                /* Happy path: We just received a frame that was in sequence */
                ev->conn->rcv_state = (ev->conn->rcv_state + 1) % ev->conn->window_size;
                ev->conn->rej_exception = false;
                if (ev->conn->srej_exception > 0)
                    ev->conn->srej_exception--;

                buffer_t *buf;
                dl_data_indication(ev, ev->info, ev->info_len);
                while ((buf = ev->conn->srej_queue[ev->conn->rcv_state])) {
                    ev->conn->srej_queue[ev->conn->rcv_state] = NULL;

                    dl_data_indication(ev, buf->buffer, buf->len);
                    buffer_free(buf);
                    ev->conn->rcv_state = (ev->conn->rcv_state + 1) % (ev->conn->window_size);
                }

                if (ev->p) {
                    ev->f = true;
                    send_rr(ev, TYPE_RES, ev->f);
                    ev->conn->ack_pending = false;
                    timer_stop_t2(ev);
                } else if (!ev->conn->ack_pending) {
                    timer_start_t2(ev);
                    ev->conn->ack_pending = true;
                    timer_start_t2(ev);
                }
                break;
            }

            if (ev->conn->rej_exception) {
                /* discard contents of I frame */
                if (ev->p) {
                    ev->f = true;
                    send_rr(ev, TYPE_RES, ev->f);
                    ev->conn->ack_pending = false;
                    timer_stop_t2(ev);
                }
                break;
            }

            if (!ev->conn->srej_enabled) {
                /* REJ frame */
                /* discard contents of I frame */
                ev->conn->rej_exception = true;
                ev->f = ev->p;
                send_rej(ev, TYPE_RES);
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                break;
            }

            /* SREJ support */
            ev->conn->srej_queue[ev->nr] = buffer_allocate(ev->info, ev->info_len);

            if (ev->conn->srej_exception > 0) {
                ev->nr = ev->ns;
                ev->f = false;
                ev->conn->srej_exception += 1;
                send_srej(ev, TYPE_RES);
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                break;
            }

            if (ev->ns == (ev->conn->rcv_state + 1) % (ev->conn->window_size)) {
                ev->nr = ev->conn->rcv_state;
                ev->f = true;
                ev->conn->srej_exception += 1;
                send_srej(ev, TYPE_RES);
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                break;
            } else {
                /* If there are two or more frames missing, give up and use REJ instead of SREJ (6.4.4.3) */
                /* discard contents of i frame */
                ev->conn->rej_exception = true;
                ev->f = ev->p;
                send_rej(ev, TYPE_RES);
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                break;
            }

    }
}

static void ax25_dl_timer_recovery(ax25_dl_event_t *ev) {
    switch (ev->event) {
        case EV_CTRL_ERROR:
            dl_error(ev, ERR_L);
            discard_queue(ev->conn);
            establish_data_link(ev);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_INFO_NOT_PERMITTED:
            dl_error(ev, ERR_M);
            discard_queue(ev->conn);
            establish_data_link(ev);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_INCORRECT_LENGTH:
            dl_error(ev, ERR_N);
            discard_queue(ev->conn);
            establish_data_link(ev);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_DL_CONNECT:
            discard_queue(ev->conn);
            establish_data_link(ev);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_DL_DISCONNECT:
            discard_queue(ev->conn);
            ev->conn->rc = 0;
            send_disc(ev, /* p= */ true);
            timer_stop_t3(ev);
            timer_start_t1(ev);
            set_state(ev->conn, STATE_AWAITING_RELEASE);
            break;

        case EV_DL_DATA: {
            buffer_t *buf = buffer_allocate(ev->info, ev->info_len);
            push_i(ev->conn, buf);
            break;
         }

        case EV_DRAIN_SENDQ:
            if (ev->conn->peer_busy || ev->conn->snd_state == ev->conn->ack_state + ev->conn->window_size) {
                /* Don't dequeue packet */
                break;
            }

            ev->ns = ev->conn->snd_state;
            ev->nr = ev->conn->rcv_state;
            ev->p = false;

            buffer_t *buf = pop_queue(ev->conn);
            packet_t *pkt = construct_i(ev, buf->buffer, buf->len, ev->nr);
            kiss_xmit(ev->conn->port, pkt->buffer, pkt->len);
            buffer_free(buf);
            if (ev->conn->sent_buffer[ev->ns]) {
                packet_free(&ev->conn->sent_buffer[ev->ns]);
            }
            ev->conn->sent_buffer[ev->ns] = pkt;


            ev->conn->ack_pending = false;
            timer_stop_t2(ev);
            ev->conn->snd_state += 1;

            if (!timer_running_t1(ev)) {
                timer_stop_t3(ev);
                timer_start_t1(ev);
            }

            break;

        case EV_TIMER_EXPIRE_T1:
            if (ev->conn->rc != ev->conn->n2) {
                ev->conn->rc = ev->conn->rc + 1;

                transmit_inquiry(ev);

                break;
            }

            if (ev->conn->ack_state == ev->conn->snd_state) {
                if (ev->conn->peer_busy) {
                    dl_error(ev, ERR_T);
                } else {
                    dl_error(ev, ERR_U);
                }
            } else {
                dl_error(ev, ERR_I);
            }

            /* dl_disconnect_request */

            discard_queue(ev->conn);

            send_dm(ev, ev->f, false);

            timer_stop_t1(ev); // Missing from SDL
            timer_stop_t3(ev); // Missing from SDL
            set_state(ev->conn, STATE_DISCONNECTED);
            break;

       case EV_SABM:
       case EV_SABME:
            if (ev->event == EV_SABME) {
                set_version_2_2(ev);
            } else {
                set_version_2_0(ev);
            }

            ev->f = ev->p;
            send_ua(ev, false);

            clear_exception_conditions(ev);

            dl_error(ev, ERR_F);

            if (ev->conn->snd_state != ev->conn->ack_state) {
                discard_queue(ev->conn);
                dl_connect_indication(ev);
            }

            timer_stop_t1(ev);
            timer_start_t3(ev);

            ev->conn->snd_state = ev->conn->ack_state = ev->conn->rcv_state = 0;

            set_state(ev->conn, STATE_CONNECTED);
            break;

       case EV_RR:
       case EV_RNR:

            if (ev->event == EV_RR) {
                ev->conn->peer_busy = false;
            } else {
                ev->conn->peer_busy = true;
            }

            if (ev->type == TYPE_RES && ev->f) {
                timer_stop_t1(ev);
                select_t1(ev);
                if (seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                    ev->conn->ack_state = ev->nr;
                    if (ev->conn->snd_state == ev->conn->rcv_state) {
                        timer_start_t3(ev);
                        set_state(ev->conn, STATE_CONNECTED);
                    } else {
                        invoke_retransmission(ev);
                        set_state(ev->conn, STATE_TIMER_RECOVERY);
                    }
                } else {
                    nr_error_recovery(ev);
                    set_state(ev->conn, STATE_AWAITING_CONNECTION);
                }
                break;
            } else {
                if (ev->type == TYPE_CMD && ev->p) {
                    enquiry_response(ev, true);
                }

                if (seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                    ev->conn->ack_state = ev->nr;
                    break;
                } else {
                    nr_error_recovery(ev);
                    set_state(ev->conn, STATE_AWAITING_CONNECTION);
                    break;
                }
            }

       case EV_DISC:
            discard_queue(ev->conn);
            ev->f = ev->p;

            send_ua(ev, false);

            dl_disconnect_indication(ev);

            timer_stop_t1(ev);
            timer_stop_t3(ev);

            set_state(ev->conn, STATE_DISCONNECTED);
            break;

       case EV_UA:
            dl_error(ev, ERR_C);

            establish_data_link(ev);

            ev->conn->l3_initiated = false;

            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_TIMER_EXPIRE_T2:
            if (ev->conn->ack_pending) {
                ev->conn->ack_pending = false;
                enquiry_response(ev, false);
            }
            timer_stop_t2(ev);
            break;

        case EV_UI:
            ui_check(ev);
            if (ev->p) {
                enquiry_response(ev, ev->f);
            }
            break;

        case EV_TEST:
            if (ev->type == TYPE_CMD) {
                send_test(ev, TYPE_RES, ev->f);
            }
            break;

        case EV_DL_UNIT_DATA:
            send_ui(ev, TYPE_CMD);
            break;

        case EV_REJ:
            ev->conn->peer_busy = false;

            if (ev->type == TYPE_RES && ev->f) {
                timer_stop_t1(ev);
                select_t1(ev);
            } else if (ev->type == TYPE_CMD && ev->p) {
                enquiry_response(ev, ev->f);
            }

            if (!seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
                break;
            }

            if (ev->conn->snd_state != ev->conn->ack_state) {
                invoke_retransmission(ev);
                set_state(ev->conn, STATE_TIMER_RECOVERY);
                break;
            }

            if (ev->type == TYPE_RES && ev->f) {
                timer_start_t3(ev);
                set_state(ev->conn, STATE_CONNECTED);
                break;
            } else {
                set_state(ev->conn, STATE_TIMER_RECOVERY);
                break;
            }

       case EV_DM:
            dl_error(ev, ERR_E);

            dl_disconnect_indication(ev);

            discard_queue(ev->conn);

            timer_stop_t1(ev);
            timer_stop_t3(ev);

            set_state(ev->conn, STATE_DISCONNECTED);
            break;

       case EV_DL_FLOW_OFF:
            if (!ev->conn->self_busy) {
                ev->conn->self_busy = true;

                send_rnr(ev, TYPE_CMD, false);

                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
            }

            break;

        case EV_DL_FLOW_ON:
            if (ev->conn->self_busy) {
                ev->conn->self_busy = false;

                send_rr(ev, TYPE_CMD, true);

                ev->conn->ack_pending = false;
                timer_stop_t2(ev);

                if (!timer_running_t1(ev)) {
                    timer_stop_t3(ev);
                    timer_start_t1(ev);
                }
            }
            break;

        case EV_FRMR:
            dl_error(ev, ERR_K);

            establish_data_link(ev);

            ev->conn->l3_initiated = false;

            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_SREJ:
            ev->conn->peer_busy = false;

            if (ev->type == TYPE_RES) {
                timer_stop_t1(ev);
                select_t1(ev);
            }

            if (!seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
                break;
            }

            if ((ev->type == TYPE_RES && ev->f) || (ev->type == TYPE_CMD && ev->p)) {
                ev->conn->ack_state = ev->nr;
            }

            if (ev->conn->ack_state != ev->conn->snd_state) {
                push_old_i_frame_nr_on_queue(ev);
                break;
            }

            if (ev->type == TYPE_RES) {
                timer_start_t3(ev);
                set_state(ev->conn, STATE_CONNECTED);
                break;
            }
            break;

       case EV_I:
            if (ev->type != TYPE_CMD) {
                dl_error(ev, ERR_S);
                break;
            }

            if (ev->info_len >= ev->conn->n1) {
                dl_error(ev, ERR_O);
                establish_data_link(ev);
                ev->conn->l3_initiated = false;
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
                break;
            }

            if (!seqno_in_range(ev->conn->ack_state, ev->nr, ev->conn->snd_state)) {
                /* recieved ack out of window */
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
                break;
            }

            ev->conn->ack_state = ev->nr;

            if (ev->conn->self_busy) {
                /* discard contents of i frame */
                if (ev->p) {
                    ev->f = 1;
                    ev->nr = ev->conn->rcv_state;
                    send_rnr(ev, TYPE_RES, ev->f);
                    ev->conn->ack_pending = false;
                    timer_stop_t2(ev);
                }
                break;
            }

            if (ev->ns == ev->conn->rcv_state) {
                /* Happy path: We just received a frame that was in sequence */
                ev->conn->rcv_state = (ev->conn->rcv_state + 1) % ev->conn->window_size;
                ev->conn->rej_exception = false;
                if (ev->conn->srej_exception > 0)
                    ev->conn->srej_exception--;

                buffer_t *buf;
                dl_data_indication(ev, ev->info, ev->info_len);
                while ((buf = ev->conn->srej_queue[ev->conn->rcv_state])) {
                    ev->conn->srej_queue[ev->conn->rcv_state] = NULL;

                    dl_data_indication(ev, buf->buffer, buf->len);
                    buffer_free(buf);
                    ev->conn->rcv_state = (ev->conn->rcv_state + 1) % (ev->conn->window_size);
                }

                if (ev->p) {
                    ev->f = true;
                    send_rr(ev, TYPE_RES, ev->f);
                    ev->conn->ack_pending = false;
                    timer_stop_t2(ev);
                } else if (!ev->conn->ack_pending) {
                    timer_start_t2(ev);
                    ev->conn->ack_pending = true;
                }
                break;
            }

            if (ev->ns == (ev->conn->rcv_state + 1) % (ev->conn->window_size)) {
                ev->nr = ev->conn->rcv_state;
                ev->f = true;
                ev->conn->srej_exception += 1;
                send_srej(ev, TYPE_RES);
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                break;
            } else {
                /* If there are two or more frames missing, give up and use REJ instead of SREJ (6.4.4.3) */
                /* discard contents of i frame */
                ev->conn->rej_exception = true;
                ev->f = ev->p;
                send_rej(ev, TYPE_RES);
                ev->conn->ack_pending = false;
                timer_stop_t2(ev);
                break;
            }
    }
}

static void ax25_dl_awaiting_connection_2_2(ax25_dl_event_t *ev) {
    (void) ev;
    UNIMPLEMENTED();
}

void ax25_dl_event(ax25_dl_event_t *ev) {
    DEBUG(D8(conn_get_state(ev->conn)), STR(": "), D8(ev->event));
    switch (conn_get_state(ev->conn)) {
        case STATE_DISCONNECTED: ax25_dl_disconnected(ev); break;
        case STATE_AWAITING_CONNECTION: ax25_dl_awaiting_connection(ev); break;
        case STATE_AWAITING_RELEASE: ax25_dl_awaiting_release(ev); break;
        case STATE_CONNECTED: ax25_dl_connected(ev); break;
        case STATE_TIMER_RECOVERY: ax25_dl_timer_recovery(ev); break;
        case STATE_AWAITING_CONNECT_2_2: ax25_dl_awaiting_connection_2_2(ev); break;
    }
}

static const char *ax25_dl_errmsg[] = {
    [ERR_A] = "F=1 received but P=1 not outstanding.",
    [ERR_B] = "Unexpected DM with F=1 in states CONNECTED(3), TIMER_RECOVERY(4) or AWAITING_CONNECT_2_2(5).",
    [ERR_C] = "Unexpected UA in states CONNECTED(3), TIMER_RECOVERY(4) or AWAITING_CONNECT_2_2(5).",
    [ERR_D] = "UA received without F=1 when SABM or DISC was sent P=1.",
    [ERR_E] = "DM received in states CONNECTED(3), TIMER_RECOVERY(4) or AWAITING_CONNECT_2_2(5).",
    [ERR_F] = "Data link reset; i.e., SABM received in state CONNECTED(3), TIMER_RECOVERY(4) or AWAITING_CONNECT_2_2(5).",
    [ERR_G] = "(G is not defined): Connection timed out",
    [ERR_H] = "(H is not defiend): Connection timed out while disconnecting",
    [ERR_I] = "N2 timeouts: unacknowledged data.",
    [ERR_J] = "N(r) sequence error.",
    [ERR_K] = "(K is not defined): FRMR received",
    [ERR_L] = "Control field invalid or not implemented.",
    [ERR_M] = "Information field was received in a U- or S-type frame.",
    [ERR_N] = "Length of frame incorrect for frame type.",
    [ERR_O] = "I frame exceeded maximum allowed length.",
    [ERR_P] = "N(s) out of the window.",
    [ERR_Q] = "UI response received, or UI command with P=1 received.",
    [ERR_R] = "UI frame exceeded maximum allowed length.",
    [ERR_S] = "I response received",
    [ERR_T] = "N2 Timeouts: no reponse to enquiry",
    [ERR_U] = "N2 Timeouts: extended peer busy",
    [ERR_V] = "No DL machines available to establish connection",
};

const char *ax25_dl_strerror(ax25_dl_error_t err) {
    if (err < 0 || err > sizeof(ax25_dl_errmsg) / sizeof(ax25_dl_errmsg[0])) {
        return "Unknown Error";
    }
    return ax25_dl_errmsg[err] ?: "Unknown error";
}
