cmake_minimum_required(VERSION 3.13)

# Pull in the Pico SDK
include(pico_sdk_import.cmake)

project(traffic_light_toy C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the Pico SDK
pico_sdk_init()

# Create the executable
add_executable(ranging_led_test15
    src/main.c

    lib/vl53l1x/src/VL53L1X_api.c
    lib/vl53l1x/src/VL53L1X_platform.c
    lib/vl53l1x/src/VL53L1X_calibration.c
)

target_include_directories(ranging_led_test15 PRIVATE lib/vl53l1x/include)

# Link against the Pico standard library
target_link_libraries(ranging_led_test15 pico_stdlib hardware_i2c)

# Create UF2, ELF, etc.
pico_add_extra_outputs(ranging_led_test15)
