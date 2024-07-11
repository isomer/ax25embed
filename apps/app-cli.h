/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * API for the CLI application
 */
#ifndef APP_CLI_H
#define APP_CLI_H
#include "debug.h"
#include "tty.h"
#include "token.h"
#include "ax25_dl.h"

void cli_init(token_t cmdline);
void caseflip_init(token_t cmdline);
void cli_output(dl_socket_t *sock, const char *msg, size_t len);

void cmd_connect_init(void);
void cmd_register_init(void);
void cmd_serial_init(void);

#endif
