cmake_minimum_required(VERSION 3.22)
project(dils C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 11)

file(GLOB C_SOURCES *.c)

add_executable(dils main.cpp
        bitmap.c
        driver.c
        driver.c)

target_sources(dils PRIVATE ${C_SOURCES})