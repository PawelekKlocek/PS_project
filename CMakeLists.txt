cmake_minimum_required(VERSION 3.26)
project(PS_project C)

set(CMAKE_C_STANDARD 11)

add_executable(PS_project
        server.c
        client.c)
