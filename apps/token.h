/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A token object, similar to rust's &[u8], or C++'s std::string_view
 */
#ifndef TOKEN_H
#define TOKEN_H
#include <sys/types.h>
#include <stdint.h>
#include "ssid.h"

/* A token object, similar to rust's &[u8], or C++'s std::string_view */
typedef struct token_t {
    const uint8_t *ptr;
    size_t len;
} token_t;

size_t my_strlen(const char *str);

/* create a new token from a pointer and an offset */
static inline token_t token_new(const uint8_t *ptr, size_t len) { return (token_t) { .ptr = ptr, .len=len }; }
/* create a new token from a C null terminated string */
static inline token_t token_from_str(const char *strz) { return token_new((uint8_t*)strz, my_strlen(strz)); }

/* Pull off len bytes from the beginning of the token, and point to them in dest */
bool token_get_bytes(token_t *source, token_t *dest, size_t len);
/* dest will point to the prefix until the first whitespace, source will be updated to point to the whitespace beyond token */
bool token_get_word(token_t *source, token_t *dest);
/* token will be updated to skip over any whitespace */
void skipwhite(token_t *token);

/* compare two tokens, return <0 if lhs<rhs, 0 if lhs==rhs, >0 if lhs>rhs */
ssize_t token_cmp(const token_t lhs, const token_t rhs);

/* Read a uint8_t from the token, returning false on failure */
bool token_get_u8(token_t *source, uint8_t *dest);

bool token_get_ssid(token_t *source, ssid_t *ssid);

#endif
