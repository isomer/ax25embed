/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Debug output helpers.
 *
 * These functions should not need to be specific per platform.
 */
#include "debug.h"
#include "platform.h"

#define RETURN_IF_FALSE(x) do { if(!(x)) return false; } while(0)


bool format_putch(char **buffer, size_t *buffer_len, char ch) {
    if (*buffer_len > 0) {
        *((*buffer)++) = ch;
        (*buffer_len)--;
        return true;
    }
    return false;
}

static const char hexit[16] = "0123456789ABCDEF";

bool format_internal_x8(char **buffer, size_t *buffer_len, struct format_t *v) {
    RETURN_IF_FALSE(format_putch(buffer, buffer_len, hexit[v->u8 >> 4]));
    return format_putch(buffer, buffer_len, hexit[v->u8 & 0x0F]);
}

static bool format_internal_int_recursive(char **buffer, size_t *buffer_len, int i) {
    if (i < 0) {
        RETURN_IF_FALSE(format_putch(buffer, buffer_len, '-'));
        i = -i;
    }
    if (i > 9) {
        RETURN_IF_FALSE(format_internal_int_recursive(buffer, buffer_len, i / 10));
        i %= 10;
    }
    return format_putch(buffer, buffer_len, '0' + i);
}

bool format_internal_int(char **buffer, size_t *buffer_len, struct format_t *v) {
    return format_internal_int_recursive(buffer, buffer_len, v->i);
}

bool format_internal_str(char **buffer, size_t *buffer_len, struct format_t *v) {
    for (const char *cp = v->ptr; *cp; cp++) {
        RETURN_IF_FALSE(format_putch(buffer, buffer_len, *cp));
    }
    return true;
}

bool format_internal_buffer(char **buffer, size_t *buffer_len, struct format_t *v) {
    for (size_t idx = 0; idx < v->buffer.len; ++idx) {
        uint8_t byte = ((uint8_t *)v->buffer.ptr)[idx];
        RETURN_IF_FALSE(format_putch(buffer, buffer_len, hexit[byte >> 4]));
        RETURN_IF_FALSE(format_putch(buffer, buffer_len, hexit[byte & 0x0F]));
        RETURN_IF_FALSE(format_putch(buffer, buffer_len, ' '));
    }
    return true;
}

bool format_internal_lenstr(char **buffer, size_t *buffer_len, struct format_t *v) {
    for (size_t idx = 0; idx < v->buffer.len; ++idx) {
        uint8_t ch = ((uint8_t *)v->buffer.ptr)[idx];
        RETURN_IF_FALSE(format_putch(buffer, buffer_len, ch));
    }
    return true;
}

bool format_internal_eol(char **buffer, size_t *buffer_len) {
    return format_putch(buffer, buffer_len, '\n');
}

void debug_putbuf(const char *buf, size_t buflen) {
    for(size_t i=0; i<buflen; ++i) {
        debug_putch(buf[i]);
    }
}

