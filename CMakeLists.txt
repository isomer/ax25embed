cmake_minimum_required(VERSION 3.6)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -g -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -pedantic -std=c2x")

project(ax25embed)


add_subdirectory(platform)
add_subdirectory(ax25)
add_subdirectory(apps)

