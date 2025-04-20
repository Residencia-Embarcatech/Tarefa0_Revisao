# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Tarefa0_Revisao C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Tarefa0_Revisao Tarefa0_Revisao.c inc/ssd1306.c)

pico_set_program_name(Tarefa0_Revisao "Tarefa0_Revisao")
pico_set_program_version(Tarefa0_Revisao "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Tarefa0_Revisao 1)
pico_enable_stdio_usb(Tarefa0_Revisao 1)

pico_generate_pio_header(Tarefa0_Revisao ${CMAKE_CURRENT_LIST_DIR}/pio_matrix.pio)

# Add the standard library to the build
target_link_libraries(Tarefa0_Revisao
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Tarefa0_Revisao PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(Tarefa0_Revisao 
        hardware_adc
        hardware_pwm
        hardware_uart
        hardware_i2c
        hardware_pio
        hardware_pwm
        )

pico_add_extra_outputs(Tarefa0_Revisao)

