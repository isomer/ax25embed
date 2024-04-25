cmake_minimum_required(VERSION 3.6)

project(ax25embed
    LANGUAGES C
)

add_executable(app-cli
    app-cli.c
    app-caseflip.c
)

target_link_libraries(app-cli ax25)


