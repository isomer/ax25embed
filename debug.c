/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Debug output
 */
#include "debug.h"
#include "platform.h"


void debug_internal_x8(struct debug_t *v) {
    static const char hexit[16] = "0123456789ABCDEF";
    debug_putch(hexit[v->u8 >> 4]);
    debug_putch(hexit[v->u8 & 0x0F]);
}

static void debug_internal_int_recursive(int i) {
    if (i < 0) {
        debug_putch('-');
        i = -i;
    }
    if (i > 9) {
        debug_internal_int_recursive(i / 10);
        i %= 10;
    }
    debug_putch('0' + i);
}

void debug_internal_int(struct debug_t *v) {
    debug_internal_int_recursive(v->i);
}

void debug_internal_str(struct debug_t *v) {
    for (const char *cp = v->ptr; *cp; cp++) {
        debug_putch(*cp);
    }
}

void debug_internal_eol(void) {
    debug_putch('\n');
}

