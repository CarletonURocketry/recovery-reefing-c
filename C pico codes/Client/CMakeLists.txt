
cmake_minimum_required(VERSION 3.13)

# Set the project name
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake) #must be before project name

project(ClientSocket C CXX ASM)

# Initialize the Pico SDK
pico_sdk_init()

# Add your executable
add_executable(ClientSocket ClientSocket.c)
pico_set_program_name(ClientSocket "ClientSocket")
pico_set_program_version(ClientSocket "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(buzzer 0)
pico_enable_stdio_usb(buzzer 1)

# Link with the necessary libraries
target_link_libraries(ClientSocket 
    pico_stdlib
    hardware_pwm
    hardware_flash
    hardware_sync
    lwip
    cyw43_arch
)

target_include_directories(ClientSocket PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(ClientSocket 
        
        )

# Create the UF2 file for flashing
pico_add_extra_outputs(ClientSocket)
