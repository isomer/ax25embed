cmake_minimum_required(VERSION 3.6)

project(ax25embed
    LANGUAGES C
)

add_executable(app-cli
    app-caseflip.c
    app-cli.c
    cmd-connect.c
    cmd-help.c
    cmd-register.c
    cmd-serial.c
    cmd.c
    console.c
    token.c
    tty.c
)

target_link_libraries(app-cli ax25)


