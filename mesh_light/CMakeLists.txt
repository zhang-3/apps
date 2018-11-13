cmake_minimum_required(VERSION 3.8)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

include_directories($ENV{ZEPHYR_BASE}/ext/hal/unisoc/)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/shell.c)
target_sources(app PRIVATE src/mesh.c)
target_sources(app PRIVATE src/light.c)
target_sources(app PRIVATE src/health.c)
