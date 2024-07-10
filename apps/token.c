/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A token object, similar to rust's &[u8], or C++'s std::string_view
 */
#include "token.h"
#include "debug.h"
#include <sys/types.h>
#include <stdint.h>
#include <string.h> // for memcmp

size_t my_strlen(const char *str) {
    size_t len = 0;
    while (*str) {
        len++;
        str++;
    }
    return len;
}

static void token_peak(const token_t *source, token_t *dest, size_t len) {
    dest->ptr = source->ptr;
    dest->len = len < source->len ? len : source->len;
}

static void token_consume(token_t *source, size_t len) {
    CHECK(len <= source->len);

    source->ptr += len;
    source->len -= len;
}

bool token_get_bytes(token_t *source, token_t *dest, size_t len) {
    token_peak(source, dest, len);
    token_consume(source, len);
    return dest->len != 0;
}

static bool token_peek_byte(const token_t source, uint8_t *dest) {
    if (source.len > 0) {
        *dest = *source.ptr;
        return true;
    } else {
        return false;
    }
}

static bool token_get_byte(token_t *source, uint8_t *dest) {
    if (token_peek_byte(*source, dest)) {
        source->ptr++;
        source->len--;
        return true;
    } else {
        return false;
    }
}

bool token_get_u8(token_t *source, uint8_t *dest) {
    bool ret = false;
    uint8_t ch;
    *dest = 0;
    skipwhite(source);
    while (token_peek_byte(*source, &ch) && ch >= '0' && ch <= '9') {
        if (!token_get_byte(source, &ch))
            return false;
        if (*dest > UINT8_MAX / 10)
            return false;
        *dest *= 10;
        if (*dest + ch - '0' < *dest)
            return false;
        *dest += ch - '0';
        ret = true; /* We consumed at least one digit */
    }

    return ret;
}

static inline bool is_whitespace(uint8_t ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

void skipwhite(token_t *token) {
    while (token->len > 0 && is_whitespace(*token->ptr)) {
        token_consume(token, 1);
    }
}

bool token_get_word(token_t *source, token_t *dest) {
    skipwhite(source);
    dest->ptr = source->ptr;
    dest->len = 0;
    while (source->len > 0 && !is_whitespace(*source->ptr)) {
        dest->len++;
        token_consume(source, 1);
    }
    return dest->len != 0;
}

ssize_t token_cmp(const token_t lhs, const token_t rhs) {
    int ret = memcmp(lhs.ptr, rhs.ptr, lhs.len < rhs.len ? lhs.len : rhs.len);
    if (ret != 0) {
        return ret;
    }
    return (ssize_t)rhs.len - (ssize_t)lhs.len;
}

bool token_get_ssid(token_t *source, ssid_t *ssid) {
    token_t ssid_token;
    if (!token_get_word(source, &ssid_token)) {
        DEBUG(STR("Failed to get word"));
        return false;
    }
    return ssid_from_string((const char *)ssid_token.ptr, ssid);
}
