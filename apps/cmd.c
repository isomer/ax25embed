/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The "command" processing.
 */
#include "cmd.h"

static command_t *commands = NULL;

void register_cmd(command_t *command) {
    command->next = commands;
    commands = command;
}


void cmd_run(struct terminal_t *term, token_t data) {
    token_t cmd;
    DEBUG(STR("Command: "), LENSTR(data.ptr, data.len));

    skipwhite(&data);
    if (!token_get_word(&data, &cmd)) {
        OUTPUT(term, STR("Failed to read command"));
        return;
    }

    for(command_t *it = commands; it; it=it->next) {
        if (token_cmp(cmd, token_from_str(it->name)) == 0) {
            it->cmd(term, data);
            return;
        }
    }

    OUTPUT(term, STR("Unknown command: ["), LENSTR(cmd.ptr, cmd.len), STR("]"));
}

