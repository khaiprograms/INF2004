# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(SD_Card C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(SD_Card 
    main.c
)

# Tell CMake where to find other source code
add_subdirectory(C:/Users/stzj0/Documents/Pico-v1.5.1/pico-examples/SDCard/no-OS-FatFS-SD-SPI-RPi-Pico-master/FatFs_SPI build)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(SD_Card)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(SD_Card 
    pico_stdlib
    FatFs_SPI
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(SD_Card 1)
pico_enable_stdio_uart(SD_Card 0)

# if (TARGET tinyusb_device)
#     add_executable(SD_Card
#             main.c
#             )

#     add_subdirectory(C:/Users/stzj0/Documents/Pico-v1.5.1/pico-examples/SD_Card/no-OS-FatFS-SD-SPI-RPi-Pico-master/FatFs_SPI build)

#     # pull in common dependencies
#     target_link_libraries(SD_Card pico_stdlib FatFs_SPI)

#     # enable usb output, disable uart output
#     pico_enable_stdio_usb(SD_Card 1)
#     pico_enable_stdio_uart(SD_Card 0)

#     # create map/bin/hex/uf2 file etc.
#     pico_add_extra_outputs(SD_Card)

#     # add url via pico_set_program_url
#     example_auto_set_url(SD_Card)
# elseif(PICO_ON_DEVICE)
#     message(WARNING "not building SD_card because TinyUSB submodule is not initialized in the SDK")
# endif()