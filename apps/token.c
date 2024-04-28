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

static inline bool is_whitespace(uint8_t ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

void skipwhite(token_t *token) {
    while (token->len > 0 && is_whitespace(*token->ptr)) {
        token_consume(token, 1);
    }
}

bool token_get_word(token_t *source, token_t *dest) {
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

