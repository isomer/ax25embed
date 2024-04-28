/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * API for the CLI application
 */
#ifndef APP_CLI_H
#define APP_CLI_H
#include "debug.h"
#include "token.h"
#include "ax25_dl.h"

#define OUTPUT(sock, ...) \
    do { \
        char output_buffer[1024]; \
        char *output_ptr = &output_buffer[1]; \
        output_buffer[0] = '\xF0'; \
        size_t output_buffer_len = sizeof(output_buffer)-1; \
        FORMAT(&output_ptr, &output_buffer_len, __VA_ARGS__); \
        cli_output((sock), output_buffer, sizeof(output_buffer) - output_buffer_len); \
    } while(0)

typedef struct command_t {
    struct command_t *next;
    const char *name;
    void (*cmd)(dl_socket_t *sock, token_t cmdline);
} command_t;

void cli_output(dl_socket_t *sock, const char *msg, size_t len);
void register_cmd(command_t *command);

void cmd_connect_init(void);
void cmd_register_init(void);

#endif
