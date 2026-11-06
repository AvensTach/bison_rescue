cmake_minimum_required(VERSION 4.2)
project(bison_rescue_clean C)

set(CMAKE_C_STANDARD 99)

add_executable(bison_rescue_clean main.c
        common.h)
