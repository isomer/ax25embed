/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Platform specific code.
 */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>

__attribute__((noreturn)) void panic(const char *dbg_msg);
void debug(const char *dbg_msg);
void debug_putch(char ch);

#define CHECK(cond) do { if (!(cond)) panic("CHECK failure: " #cond); } while(0)

struct debug_t {
    void (*fmt)(struct debug_t *self);
    union {
        const void *ptr;
        uint8_t u8;
    };
};

void debug_internal_d8(struct debug_t *self);
static inline struct debug_t debug_d8(uint8_t v) { return (struct debug_t) { .fmt = debug_internal_d8, .u8 = v }; }
void debug_internal_x8(struct debug_t *self);
static inline struct debug_t debug_x8(uint8_t v) { return (struct debug_t) { .fmt = debug_internal_x8, .u8 = v }; }
void debug_internal_str(struct debug_t *self);
static inline struct debug_t debug_str(const char *v) { return (struct debug_t) { .fmt = debug_internal_str, .ptr = v }; }
void debug_internal_eol(void);

#define D8(v) debug_d8(v)
#define X8(v) debug_x8(v)
#define STR(v) debug_str(v)

#define DEBUG1(v) do { struct debug_t debug_fmt_value = (v); debug_fmt_value.fmt(&debug_fmt_value); } while(0)
#define DEBUG2(v, ...) DEBUG1(v); DEBUG1(__VA_ARGS__)
#define DEBUG3(v, ...) DEBUG1(v); DEBUG2(__VA_ARGS__)
#define DEBUG4(v, ...) DEBUG1(v); DEBUG3(__VA_ARGS__)
#define DEBUG5(v, ...) DEBUG1(v); DEBUG4(__VA_ARGS__)
#define DEBUG6(v, ...) DEBUG1(v); DEBUG5(__VA_ARGS__)

#define DEBUG_COUNT_ARGS(a1, a2, a3, a4, a5, a6, cmd, ...) cmd

/* How to use this macro, give it up to 6 arguments.  each argument must be a
 * macro saying the type of argument and it's value.  This should be type safe
 * (if a bit ugly).  Convention for integers is "D" for decimal output, "X" for
 * "hex" output, optionally followed by "U" for unsigned, then the bitwidth.
 *
 * eg: DEBUG(STR("The count is"), DU8(count));
 */
#define DEBUG(...) \
    do { \
        DEBUG_COUNT_ARGS(__VA_ARGS__, DEBUG6, DEBUG5, DEBUG4, DEBUG3, DEBUG2, DEBUG1)(__VA_ARGS__); \
        debug_internal_eol(); \
    } while(0)

#define UNIMPLEMENTED() \
    do { \
        DEBUG(STR("unimplemented: "), STR(__func__)); \
        panic("not continuing"); \
    } while(0)

#endif
