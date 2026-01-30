cmake_minimum_required(VERSION 3.13)

# Pull in the Pico SDK
include(pico_sdk_import.cmake)

project(traffic_light_toy C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the Pico SDK
pico_sdk_init()

# Create the executable
add_executable(ext_blink
    src/main.c
)

# Link against the Pico standard library
target_link_libraries(ext_blink pico_stdlib)

# Create UF2, ELF, etc.
pico_add_extra_outputs(ext_blink)
