cmake_minimum_required(VERSION 3.6)

project(platform)

add_library(platform STATIC
    platform-posix.c
    serial-tty.c
    vserial.c
    debug.c
    clock.c
)

target_include_directories(platform PUBLIC public)


