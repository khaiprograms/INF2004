cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)

project(pico_w_webserver)

set(PICO_BOARD pico_w)

# Initialize the SDK
pico_sdk_init()

add_executable(webserver
main.c
webserver.c)

target_include_directories(webserver PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
    )

# pull in common dependencies
target_link_libraries(webserver 
        pico_cyw43_arch_lwip_threadsafe_background
        pico_lwip_http
        pico_stdlib
        hardware_adc)

 # enable usb output, disable uart output
 pico_enable_stdio_usb(webserver 1)
 pico_enable_stdio_uart(webserver 0)


# create map/bin/hex file etc.
pico_add_extra_outputs(webserver)

