/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A command to register a new application
 */
#include "app-cli.h"
#include "ax25.h"
#include "ax25_dl.h"
#include "token.h"
#include "cmd.h"

static const struct apps_t {
    const char *name;
    void (*init)(token_t cmdline);
} apps[] = {
    { "cli", cli_init },
    { "caseflip", caseflip_init },
    { NULL, NULL },
};

static void cmd_register(terminal_t *term, token_t cmdline) {
    const char *help = "register <app> ...\n";
    token_t app;

    skipwhite(&cmdline);

    if (!token_get_word(&cmdline, &app)) {
        OUTPUT(term, STR(help));
        return;
    }

    skipwhite(&cmdline);

    for(size_t it = 0; apps[it].name; ++it) {
        if (token_cmp(app, token_from_str(apps[it].name)) == 0) {
            apps[it].init(cmdline);
            return;
        }
    }

    OUTPUT(term, STR("Unknown app: "), LENSTR(app.ptr, app.len));

    return;
}

static command_t command_register = {
    .next = NULL,
    .name = "register",
    .cmd = cmd_register,
};

void cmd_register_init(void) {
    register_cmd(&command_register);
}
