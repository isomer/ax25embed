cmake_minimum_required(VERSION 3.6)

project(ax25embed)

add_library(ax25 STATIC
	 ax25_dl.c
	 ax25.c
	 buffer.c
     capture.c
	 connection.c
	 kiss.c
	 metric.c
	 packet.c
	 ssid.c
)

target_include_directories(platform-posix PUBLIC public)
target_include_directories(platform-null PUBLIC public)

target_link_libraries(ax25 platform)


