#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app LANGUAGES CXX)

FILE(GLOB app_sources src/*.cpp src/*.c)
target_sources(app PRIVATE ${app_sources})


zephyr_library_include_directories(.)

target_link_libraries(app PUBLIC stdc++) 
