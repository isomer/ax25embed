/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A command to get help
 */

#include "app-cli.h"
#include "cmd.h"

/* extern from cmd.c */
extern command_t *commands;

static void cmd_help(terminal_t *term, token_t cmdline) {
    (void) cmdline; /* ignores argument */
    for(command_t *c = commands; c != NULL; c = c->next) {
        OUTPUT(term, STR(c->help));
    }
}


static command_t command_help = {
    .next = NULL,
    .name = "help",
    .help = "help",
    .cmd = cmd_help,
};

void cmd_help_init(void) {
    register_cmd(&command_help);
}
