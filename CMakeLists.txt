cmake_minimum_required(VERSION 3.20)
project(Can C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Can
        main.c
        MCP2515.c
        MCP2515.h
        UART.c)
