cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)

project(complete_project)

set(PICO_BOARD pico_w)
set(LWIP_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/lwip)

# Initialize the SDK;
pico_sdk_init()

add_executable(complete_project
main.c
sd.c
webserver.c
#${CMAKE_CURRENT_LIST_DIR}/src/packet_handler.c
)

add_library(pico_lwip_complete INTERFACE)

target_include_directories(pico_lwip_complete INTERFACE
	${LWIP_PATH}/src/include
	${CMAKE_CURRENT_LIST_DIR}/src/lwip
)

target_include_directories(complete_project PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/src/include
    )

# Tell CMake where to find other source code
add_subdirectory(no-OS-FatFS-SD-SPI-RPi-Pico-master/FatFs_SPI build)

add_library(pico_rmii_ethernet INTERFACE)

target_sources(pico_rmii_ethernet INTERFACE 
${CMAKE_CURRENT_LIST_DIR}/src/rmii_ethernet.c)

# pull in common dependencies
target_link_libraries(complete_project
       pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        pico_multicore
        hardware_adc
        FatFs_SPI
        pico_lwip_complete
        pico_rmii_ethernet
        pico_lwip_http
        )

 # enable usb output, disable uart output
pico_enable_stdio_usb(complete_project 1)
pico_enable_stdio_uart(complete_project 0)

pico_generate_pio_header(complete_project ${CMAKE_CURRENT_LIST_DIR}/src/rmii_ethernet_phy_rx.pio)
pico_generate_pio_header(complete_project ${CMAKE_CURRENT_LIST_DIR}/src/rmii_ethernet_phy_tx.pio)

# create map/bin/hex file etc.
pico_add_extra_outputs(complete_project)
