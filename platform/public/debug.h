#ifndef DEBUG_H
#define DEBUG_H 1
/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Debug output definitions
 */
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

noreturn void panic(const char *msg);  // forward definition from platform.h

struct format_t {
    bool (*fmt)(char **buffer, size_t *buffer_len, struct format_t *self);
    union {
        const void *ptr;
        uint8_t u8;
        int i;
        struct {
            const void *ptr;
            size_t len;
        } buffer;
    };
};

bool format_putch(char **buffer, size_t *buffer_len, char ch);
bool format_internal_int(char **buffer, size_t *buffer_len, struct format_t *self);
static inline struct format_t format_d8(uint8_t v) { return (struct format_t) { .fmt = format_internal_int, .i = v }; }
static inline struct format_t format_int(int v) { return (struct format_t) { .fmt = format_internal_int, .i = v }; }
bool format_internal_x8(char **buffer, size_t *buffer_len, struct format_t *self);
static inline struct format_t format_x8(uint8_t v) { return (struct format_t) { .fmt = format_internal_x8, .u8 = v }; }
bool format_internal_str(char **buffer, size_t *buffer_len, struct format_t *self);
static inline struct format_t format_str(const char *v) { return (struct format_t) { .fmt = format_internal_str, .ptr = v }; }
bool format_internal_buffer(char **buffer, size_t *buffer_len, struct format_t *self);
static inline struct format_t format_buffer(const void *buf, size_t len) { return (struct format_t) { .fmt = format_internal_buffer, .buffer = { .ptr = buf, .len = len } }; }
bool format_internal_lenstr(char **buffer, size_t *buffer_len, struct format_t *self);
static inline struct format_t format_lenstr(const void *buf, size_t len) { return (struct format_t) { .fmt = format_internal_lenstr, .buffer = { .ptr = buf, .len = len } }; }
bool format_internal_eol(char **buffer, size_t *buffer_len);

void debug_putbuf(const char *buf, size_t buflen);

#define INT(v) format_int(v)
#define D8(v) format_d8(v)
#define X8(v) format_x8(v)
#define STR(v) format_str(v)
#define BUF(ptr, len) format_buffer(ptr, len)
#define LENSTR(ptr, len) format_lenstr(ptr, len)

#define FORMAT1(buffer, buffer_len, v) do { struct format_t format_fmt_value = (v); format_fmt_value.fmt(buffer, buffer_len, &format_fmt_value); } while(0)
#define FORMAT2(buffer, buffer_len, v, ...) FORMAT1(buffer, buffer_len, v); FORMAT1(buffer, buffer_len, __VA_ARGS__)
#define FORMAT3(buffer, buffer_len, v, ...) FORMAT1(buffer, buffer_len, v); FORMAT2(buffer, buffer_len, __VA_ARGS__)
#define FORMAT4(buffer, buffer_len, v, ...) FORMAT1(buffer, buffer_len, v); FORMAT3(buffer, buffer_len, __VA_ARGS__)
#define FORMAT5(buffer, buffer_len, v, ...) FORMAT1(buffer, buffer_len, v); FORMAT4(buffer, buffer_len, __VA_ARGS__)
#define FORMAT6(buffer, buffer_len, v, ...) FORMAT1(buffer, buffer_len, v); FORMAT5(buffer, buffer_len, __VA_ARGS__)

#define FORMAT_COUNT_ARGS(a1, a2, a3, a4, a5, a6, cmd, ...) cmd

/* How to use this macro, give it up to 6 arguments.  each argument must be a
 * macro saying the type of argument and it's value.  This should be type safe
 * (if a bit ugly).  Convention for integers is "D" for decimal output, "X" for
 * "hex" output, optionally followed by "U" for unsigned, then the bitwidth.
 *
 * eg: DEBUG(STR("The count is"), DU8(count));
 */
#define FORMAT(buffer, buffer_len, ...) \
    do { \
        FORMAT_COUNT_ARGS(__VA_ARGS__, FORMAT6, FORMAT5, FORMAT4, FORMAT3, FORMAT2, FORMAT1)((buffer), (buffer_len), __VA_ARGS__); \
        format_internal_eol((buffer), (buffer_len)); \
    } while(0)

#define DEBUG(...) \
    do { \
        char debug_buffer[1024]; \
        char *debug_ptr = debug_buffer; \
        size_t debug_buffer_len = sizeof(debug_buffer); \
        FORMAT(&debug_ptr, &debug_buffer_len, __VA_ARGS__); \
        debug_putbuf(debug_buffer, sizeof(debug_buffer) - debug_buffer_len); \
    } while(0)

#define CHECK(cond) do { if (!(cond)) panic("CHECK failure: " #cond); } while(0)

#define UNIMPLEMENTED() \
    do { \
        DEBUG(STR("unimplemented: "), STR(__func__)); \
        panic("not continuing"); \
    } while(0)

#endif
