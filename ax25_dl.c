/* Based on AX.25 v2.2 (https://www.tapr.org/pdf/AX25.2.2.pdf)
 *
 * This is currently as literal translation of the SDL as I can possibly make it.
 *
 * Notes:
 *  - TEST frames aren't handled anywhere?
 */
#include "ax25_dl.h"
#include "buffer.h"
#include "platform.h"

static void send_dm(ax25_dl_event_t *ev, bool f, bool expedited) {
    (void) ev;
    (void) f;
    (void) expedited;
    panic("not implemented");
}

static void send_ui(ax25_dl_event_t *ev) {
    (void) ev;
    panic("not implemented");
}

static void send_ua(ax25_dl_event_t *ev, bool f, bool expedited) {
    (void) ev;
    (void) f;
    (void) expedited;
    panic("not implemented");
}

static void send_sabm(ax25_dl_event_t *ev, bool f) {
    (void) ev;
    (void) f;
    panic("not implemented");
}

static void send_disc(ax25_dl_event_t *ev, bool f) {
    (void) ev;
    (void) f;
    panic("not implemented");
}

static void send_srej(ax25_dl_event_t *ev, bool f, uint8_t nr) {
    (void) ev;
    (void) f;
    (void) nr;
    panic("not implemented");
}

static void send_rej(ax25_dl_event_t *ev, bool f, uint8_t nr) {
    (void) ev;
    (void) f;
    (void) nr;
    panic("not implemented");
}

static void send_rr(ax25_dl_event_t *ev, bool f, uint8_t nr) {
    (void) ev;
    (void) f;
    (void) nr;
    panic("not implemented");
}

static void send_rnr(ax25_dl_event_t *ev, bool f, uint8_t nr) {
    (void) ev;
    (void) f;
    (void) nr;
    panic("not implemented");
}

static void send_i(ax25_dl_event_t *ev, bool f, uint8_t ns, uint8_t nr) {
    (void) ev;
    (void) f;
    panic("not implemented");
}

static void establish_data_link(ax25_dl_event_t *ev, connection_t *conn) {
    (void) ev;
    (void) conn;
    panic("not implemented");
}

static void select_t1(ax25_dl_event_t *ev) {
    (void) ev;
    panic("not implemented");
}

static void discard_queue(connection_t *conn) {
    (void) conn;
    panic("not implemented");
}

static void ui_check(ax25_dl_event_t *ev) {
    (void) ev;
    panic("not implemented");
}

static void set_state(connection_t *conn, conn_state_t state) {
    conn->state = state;
    if (state == STATE_DISCONNECTED) {
        conn_release(conn);
    }
}

static bool timer_running_t1(ax25_dl_event_t *ev) {
    (void)ev;
    panic("not implemented");
}

static void timer_start_t1(ax25_dl_event_t *ev) {
    (void)ev;
    panic("not implemented");
}

static void timer_stop_t1(ax25_dl_event_t *ev) {
    (void)ev;
    panic("not implemented");
}

static void timer_start_t3(ax25_dl_event_t *ev) {
    (void)ev;
    panic("not implemented");
}

static void timer_stop_t3(ax25_dl_event_t *ev) {
    (void)ev;
    panic("not implemented");
}

static void clear_exception_conditions(ax25_dl_event_t *ev) {
    (void)ev;
    panic("not implemented");
}

static void ax25_dl_disconnected(ax25_dl_event_t *ev) {
    connection_t *conn;
    bool f;
    switch (ev->event) {
        case EV_CTRL_ERROR:
            /* TODO: dl_error_condition(L) */
            break;
        case EV_INFO_NOT_PERMITTED:
            /* TODO: dl_error_condition(M) */
            break;
        case EV_INCORRECT_LENGTH:
            /* TODO: dl_error_condition(N) */
            break;
        case EV_UA:
            /* TODO: dl_error_condition(C, D) */
            break;

        case EV_DM:
            /* set state to disconnected */
            break;

        case EV_UI:
            /* TODO: ui_check(); */
            if (ev->pf) {
                send_dm(ev, /* f= */ true, /* expedited= */ false);
            }
            break;

         case EV_DL_DISCONNECT:
            /* confirm */
            break;

         case EV_DISC:
            f = ev->pf;
            send_dm(ev, f, /* expedited=*/ false);
            break;

         case EV_DL_UNIT_DATA:
            send_ui(ev);
            break;

         /* All other commands */
         case EV_UNKNOWN_FRAME:
         case EV_I:
         case EV_RR:
         case EV_RNR:
         case EV_REJ:
         case EV_SREJ:
         case EV_FRMR:
            f = ev->pf;
            send_dm(ev, f, /* expedited= */ false);
            break;

         /* All other primatives */
         case EV_DL_DATA:
         case EV_DL_FLOW_ON:
         case EV_DL_FLOW_OFF:
         case EV_TIMER_EXPIRE_T1:
         case EV_TIMER_EXPIRE_T3:
            break;

         case EV_DL_CONNECT_REQUEST:
            conn = conn_find_or_create(&ev->address[ADDR_SRC]);
            conn->srtt = SRTT_DEFAULT;
            conn->t1v =  conn->srtt * 2;

            establish_data_link(ev, conn);

            ev->conn->l3_initiated = true;

            set_state(conn, STATE_AWAITING_CONNECTION);

            break;

         case EV_SABM:
         case EV_SABME:
            f = ev->pf;

            conn = conn_find_or_create(&ev->address[ADDR_SRC]);
            if (!conn) {
                send_dm(ev, f, /* expedited= */ false);
            } else {
                send_ua(ev, false, false);
                conn->snd_state = 0;
                conn->ack_state = 0;
                conn->rcv_state = 0;
                conn->srtt = SRTT_DEFAULT;
                conn->t1v = conn->srtt * 2;
                set_state(conn, STATE_CONNECTED);
                conn->l3_initiated = false; /* TODO: Check what this should be set to? */
                /* TODO: start T3 */
            }
            break;
    }
}

static void ax25_dl_awaiting_connection(ax25_dl_event_t *ev) {
    bool f;
    switch(ev->event) {
        case EV_CTRL_ERROR:
            /* TODO: dl_error_condition(L) */
            break;
        case EV_INFO_NOT_PERMITTED:
            /* TODO: dl_error_condition(M) */
            break;
        case EV_INCORRECT_LENGTH:
            /* TODO: dl_error_condition(N) */
            break;
        case EV_DL_CONNECT_REQUEST:
            discard_queue(ev->conn);
            ev->conn->l3_initiated = true;
            break;
        case EV_DL_DISCONNECT:
            /* TODO: requeue */
            break;
        case EV_SABM:
            f = ev->pf;
            send_ua(ev, false, false);
            break;
        case EV_DISC:
            f = ev->pf;
            send_dm(ev, f, /* expedited= */ false);
            break;
        case EV_DL_DATA:
            if (!ev->conn->l3_initiated) {
                /* TODO: queue frame */
            }
            break;

        /* case EV_POP_FRAME: TODO */
        case EV_UI:
            ui_check(ev);
            if (ev->pf) {
                send_dm(ev, true, /* expedited= */ false);
            }
            break;
         case EV_DL_UNIT_DATA:
            send_ui(ev);
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
            break;
         case EV_DM:
            if (ev->pf) {
                discard_queue(ev->conn);
                /* TODO: dl_disconnect_indication */
                timer_stop_t1(ev);

                set_state(ev->conn, STATE_DISCONNECTED);
            } else {
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
            }
            break;
         case EV_UA:
            if (ev->pf) {
                if (ev->conn->l3_initiated) {
                    /* TODO: dl_disconnect confirm */
                } else {
                    if (ev->conn->snd_state != ev->conn->ack_state) {
                        discard_queue(ev->conn);
                        /* dl connect indication */
                    }
                    timer_stop_t1(ev);
                    timer_stop_t3(ev);
                    ev->conn->snd_state = ev->conn->ack_state = ev->conn->rcv_state = 0;
                    select_t1(ev);
                    set_state(ev->conn, STATE_CONNECTED);
                }
            } else {
                /* TODO: dl_error_indication(D) */
            }
            break;

         case EV_TIMER_EXPIRE_T1:
            if (ev->conn->rc == ev->conn->n2) {
                discard_queue(ev->conn);
                /* TODO: dl_error indication(g) */
                /* dl disconnect indication */
                set_state(ev->conn, STATE_DISCONNECTED);
            } else {
                ev->conn->rc = ev->conn->rc + 1;
                send_sabm(ev, /* p=*/ 1);
                select_t1(ev);
                timer_start_t1(ev);
            }
            break;

         case EV_SABME:
            f = ev->pf;
            send_dm(ev, f, /* expedited= */ false);
            set_state(ev->conn, STATE_AWAITING_CONNECT_2_2);
            break;
    }
}

static void ax25_dl_awaiting_release(ax25_dl_event_t *ev) {
    bool f;
    switch(ev->event) {
        case EV_CTRL_ERROR:
            /* TODO: dl_error_condition(L) */
            break;
        case EV_INFO_NOT_PERMITTED:
            /* TODO: dl_error_condition(M) */
            break;
        case EV_INCORRECT_LENGTH:
            /* TODO: dl_error_condition(N) */
            break;
        case EV_DL_DISCONNECT:
            send_dm(ev, false, /* expedited= */ true);
            timer_stop_t1(ev);
            set_state(ev->conn, STATE_DISCONNECTED);
            break;
        case EV_SABM:
            f = ev->pf;
            send_dm(ev, f, /* expedited= */ true);
            break;
        case EV_DISC:
            f = ev->pf;
            send_ua(ev, f, /* expedited= */ true);
            break;
        case EV_DL_UNIT_DATA:
            send_ui(ev);
            break;
        case EV_I:
        case EV_RR:
        case EV_RNR:
        case EV_REJ:
        case EV_SREJ:
            if (ev->pf) {
                send_dm(ev, /* f= */ true, /* expedited= */ false);
            }
            break;
        case EV_UI:
            ui_check(ev);
            if (ev->pf) {
                send_dm(ev, /* f= */ true, /* expedited= */ false);
            }
            break;
        /* all other primatives */
        case EV_TIMER_EXPIRE_T3:
        case EV_DL_FLOW_ON:
        case EV_DL_FLOW_OFF:
        case EV_UNKNOWN_FRAME:
        case EV_DL_CONNECT_REQUEST:
        case EV_DL_DATA:
        case EV_SABME:
        case EV_FRMR:
            break;
        case EV_UA:
            if (ev->pf) {
                /* TODO: dl_disconnect_confirm */
                timer_stop_t1(ev);
                set_state(ev->conn, STATE_DISCONNECTED);
            } else {
                /* TODO: dl_error_indication */
            }
            break;
        case EV_DM:
            if (ev->pf) {
                /* TODO: dl_connect_confirm */
                timer_stop_t1(ev);
                set_state(ev->conn, STATE_DISCONNECTED);
            }
            break;
        case EV_TIMER_EXPIRE_T1:
            if (ev->conn->rc == ev->conn->n2) {
                /* TODO: dl_error_indication */
                /* TODO: dl_disconnect_confirm */
                set_state(ev->conn, STATE_DISCONNECTED);
            } else {
                ev->conn->rc = ev->conn->rc + 1;
                send_disc(ev, /* p= */ 1);
                select_t1(ev);
                timer_start_t1(ev);
            }
    }
}

static void ax25_dl_connected(ax25_dl_event_t *ev) {
    bool p;
    bool f;
    uint8_t ns, nr;
    switch (ev->event) {
        case EV_CTRL_ERROR:
            /* TODO: dl_error_indication(L) */
            discard_i_queue(ev->conn);
            establish_data_link(ev, ev->conn);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_INFO_NOT_PERMITTED:
            /* TODO: dl_error_indication(M) */
            discard_i_queue(ev->conn);
            establish_data_link(ev, ev->conn);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_INCORRECT_LENGTH:
            /* TODO: dl_error_indication(N) */
            discard_i_queue(ev->conn);
            establish_data_link(ev, ev->conn);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_DL_CONNECT_REQUEST:
            discard_i_queue(ev->conn);
            establish_data_link(ev, ev->conn);
            ev->conn->l3_initiated = true;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

        case EV_DL_DISCONNECT:
            discard_i_queue(ev->conn);
            ev->conn->rc = 0;
            send_disc(ev, /* p= */ true);
            timer_stop_t3(ev);
            timer_start_t1(ev);
            set_state(ev->conn, STATE_AWAITING_RELEASE);
            break;

        case EV_DL_DATA:
            push_i(ev);
            break;
        /* case pop off data from i queue */
            if (ev->conn->peer_busy
                    || ev->conn->snd_state == ev->conn->ack_state + ev->conn->window_size ) {
                push_i(ev);
            } else {
                ns = ev->conn->snd_state;
                nr = ev->conn->rcv_state;
                p = false;
                send_i(ev, /* p= */ p, ns, nr);
                ev->conn->snd_state = ev->conn->snd_state + 1;
                ev->conn->ack_pending = false;
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
            f = ev->pf;
            send_ua(ev, f, false);
            clear_exception_conditions(ev);
            /* TODO: dl_error_indication (F) */
            if (ev->conn->snd_state != ev->conn->ack_state) {
                discard_i_queue(ev);
                /* TODO: dl_connect_indication */
            }
            timer_stop_t1(ev);
            timer_start_t3(ev);
            ev->conn->snd_state = ev->conn->ack_state = ev->conn->rcv_state = 0;
            break;

       case EV_DISC:
            discard_i_queue(ev);
            f = ev->pf;
            send_ua(ev, f, /* TODO: check */ false);
            /* TODO: dl_disconnect_indication */
            timer_stop_t1(ev);
            timer_stop_t3(ev);
            set_state(ev->conn, STATE_DISCONNECTED);
            break;

       case EV_UA:
            /* TODO: dl_error_indication(C) */
            establish_data_link(ev, ev->conn);
            ev->conn->l3_initiated = false;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

       case EV_DM:
            /* TODO: dl_error_indication(E) */
            /* TODO: dl_disconnect_indication */
            discard_i_queue(ev);
            timer_stop_t1(ev);
            timer_stop_t3(ev);
            set_state(ev->conn, STATE_DISCONNECTED);
            break;

       case EV_FRMR:
            /* TODO: dl_error_indication(K) */
            establish_data_link(ev, ev->conn);
            ev->conn->l3_initiated = false;
            set_state(ev->conn, STATE_AWAITING_CONNECTION);
            break;

       case EV_DL_FLOW_OFF:
            if (!ev->conn->self_busy) {
                ev->conn->self_busy = true;
                send_rnr(ev, /* f= */ false, ev->nr);
                ev->conn->ack_pending = false;
            }
            break;

       case EV_DL_FLOW_ON:
            if (ev->conn->self_busy) {
                ev->conn->self_busy = false;
                send_rr(ev, /* p= */ true, ev->nr);
                ev->conn->ack_pending = false;
                if (!timer_running_t1(ev)) {
                    timer_stop_t3(ev);
                    timer_start_t1(ev);
                }
            }
            break;

       case EV_DL_UNIT_DATA:
            send_ui(ev);
            break;

       case EV_UI:
            ui_check(ev);
            if (ev->pf) {
                enquiry_response(ev, /* f= */ true);
            }
            break;

       case EV_RR:
            ev->conn->peer_busy = false;
            check_need_for_response(ev);
            if (ev->conn->ack_state <= ev->nr && ev->nr <= ev->conn->snd_state) {
                check_i_frame_acked(ev);
            } else {
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
            }
            break;

       case EV_RNR:
            ev->conn->peer_busy = true;
            check_need_for_response(ev);
            if (ev->conn->ack_state <= ev->nr && ev->nr <= ev->conn->snd_state) {
                check_i_frame_acked(ev);
            } else {
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
            }
            break;

        /* TODO: lm-sieze confirm! */

       case EV_SREJ:
            ev->conn->peer_busy = false;
            check_need_for_response(ev);
            if (ev->conn->ack_state <= ev->nr && ev->nr <= ev->conn->snd_state) {
                if (ev->pf) {
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
            if (ev->conn->ack_state <= ev->nr && ev->nr <= ev->conn->snd_state) {
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
            if (!true /*TODO: command?*/) {
                /* TODO: dl_error_indication(S) */
                break;
            }

            if (ev->info_len >= ev->conn->n2) {
                /* TODO: dl_error_indication(O) */
                establish_data_link(ev, ev->conn);
                ev->conn->l3_initiated = false;
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
                break;
            }

            if (!(ev->conn->ack_state <= ev->nr && ev->nr <= ev->conn->snd_state)) {
                /* recieved ack out of window */
                nr_error_recovery(ev);
                set_state(ev->conn, STATE_AWAITING_CONNECTION);
                break;
            }

            check_i_frame_acked(ev);

            if (ev->conn->self_busy) {
                /* discard contents of i frame */
                if (ev->pf) {
                    f = 1;
                    nr = ev->conn->rcv_state;
                    send_rnr(ev, f, nr);
                    ev->conn->ack_pending = false;
                }
                break;
            }

            if (ev->nr == ev->conn->rcv_state) {
                /* Happy path: We just received a frame that was in sequence */
                ev->conn->rcv_state = ev->conn->rcv_state + 1; /* TODO: mod k ? */
                ev->conn->rej_exception = false;
                if (ev->conn->srej_exception > 0)
                    ev->conn->srej_exception--;

                buffer_t *buf;
                /* dl_data_indication(ev, ev->info, ev->infolen); */
                while ((buf = ev->conn->srej_queue[ev->conn->rcv_state])) {
                    ev->conn->srej_queue[ev->conn->rcv_state] = NULL;

                    /* TODO: dl_data_indication(ev, buf->buffer, buf->len); */
                    buffer_free(buf);
                    ev->conn->rcv_state = ev->conn->rcv_state +1; /* TODO: mod k ? */
                }

                if (ev->pf) {
                    f = true;
                    send_rr(ev, f, /* nr= */ ev->conn->rcv_state);
                    ev->conn->ack_pending = true;
                } else if (!ev->conn->ack_pending) {
                    /* TODO: LM-Sieze request */
                    ev->conn->ack_pending = true;
                }
                break;
            }

            if (ev->conn->rej_exception) {
                /* discard contents of I frame */
                if (ev->pf) {
                    f = true;
                    send_rr(ev, f, /* nr= */ ev->conn->rcv_state);
                    ev->conn->ack_pending = true;
                }
                break;
            }

            /* REJ frame */
            if (!ev->conn->srej_enabled) {
                /* discard contents of I frame */
                ev->conn->rej_exception = true;
                f = ev->pf;
                send_rej(ev, f, ev->conn->rcv_state);
                ev->conn->ack_pending = false;
                break;
            }

            /* SREJ support */
            ev->conn->srej_queue[ev->nr] = buffer_allocate(ev->info, ev->info_len);

            if (ev->conn->srej_exception > 0) {
                nr = ev->ns;
                f = false;
                ev->conn->srej_exception += 1;
                send_srej(ev, f, nr);
                ev->conn->ack_pending = false;
                break;
            }

            if (ev->ns > ev->conn->rcv_state + 1) {
                /* discard contents of i frame */
                ev->conn->rej_exception = true;
                f = ev->pf;
                send_rej(ev, f, ev->conn->rcv_state);
                ev->conn->ack_pending = false;
                break;
            } else {
                nr = ev->conn->rcv_state;
                f = true;
                ev->conn->srej_exception += 1;
                send_srej(ev, f, nr);
                ev->conn->ack_pending = false;
                break;
            }

    }
}

static void ax25_dl_timer_recovery(ax25_dl_event_t *ev) {
    (void) ev;
    panic("not implemented");
}

static void ax25_dl_awaiting_connection_2_2(ax25_dl_event_t *ev) {
    (void) ev;
    panic("not implemented");
}

void ax25_dl_event(ax25_dl_event_t *ev) {
    switch (conn_get_state(ev->conn)) {
        case STATE_DISCONNECTED: ax25_dl_disconnected(ev); break;
        case STATE_AWAITING_CONNECTION: ax25_dl_awaiting_connection(ev); break;
        case STATE_AWAITING_RELEASE: ax25_dl_awaiting_release(ev); break;
        case STATE_CONNECTED: ax25_dl_connected(ev); break;
        case STATE_TIMER_RECOVERY: ax25_dl_timer_recovery(ev); break;
        case STATE_AWAITING_CONNECT_2_2: ax25_dl_awaiting_connection_2_2(ev); break;
    }
}
