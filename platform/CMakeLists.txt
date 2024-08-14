cmake_minimum_required(VERSION 3.6)

project(platform)

add_library(platform-common STATIC
    clock.c
    debug.c
    vserial.c
)

target_include_directories(platform-common PUBLIC public)

add_library(platform-posix STATIC
    pcap.c
    platform-posix.c
    serial-tty.c
)

target_link_libraries(platform-posix platform-common)

add_library(platform-null STATIC
    platform-null.c
    serial-dummy.c
)

target_link_libraries(platform-null platform-common)

set(PLATFORM "posix" CACHE STRING "Target build platform")

add_library(platform ALIAS platform-${PLATFORM})
