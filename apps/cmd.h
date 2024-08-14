/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The "command" processing API.
 */
#ifndef CMD_H
#define CMD_H
#include "debug.h"
#include "token.h"
#include "tty.h"

#define OUTPUT(term, ...) \
    do { \
        char output_buffer[1024]; \
        char *output_ptr = &output_buffer[1]; \
        output_buffer[0] = '\xF0'; \
        size_t output_buffer_len = sizeof(output_buffer)-1; \
        FORMAT(&output_ptr, &output_buffer_len, __VA_ARGS__); \
        terminal_tx((term), token_new((uint8_t *)output_buffer, sizeof(output_buffer) - output_buffer_len)); \
    } while(0)

typedef struct command_t {
    struct command_t *next;
    const char *name;
    const char *help;
    void (*cmd)(terminal_t *term, token_t cmdline);
} command_t;

void register_cmd(command_t *command);
void cmd_run(terminal_t *term, token_t data);

#endif
