cmake_minimum_required(VERSION 3.10)
project(bison_rescue_clean C)

set(CMAKE_C_STANDARD 99)

add_executable(bison_rescue_clean
        main.c
        common.h
        file_writer.c
        file_writer.h
        ipc_sync.c
        ipc_sync.h)