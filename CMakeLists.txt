cmake_minimum_required(VERSION 3.12)

# LSP support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set board type because we are building for PicoW
set(PICO_BOARD pico_w)

# Name project
SET(ProjectName greenHouse)

# Set any variables required for importing libraries
SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-KernelV10.6.2)

# Import those libraries
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Define project
project(${ProjectName})

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

#add_compile_options(-O0)
# Add subdirectories
add_subdirectory(src)

# if you have anything in "lib" folder then uncomment below - remember to add a CMakeLists.txt
# file to the "lib" directory
#add_subdirectory(lib)

target_include_directories(greenHouse
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/systemTasks"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/sensors"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FanDriver"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/ValveDriver"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/UI"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/cloud"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/controller"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/FanDriver"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/EEPROM"

)

set(CMAKE_SYSTEM_NAME Generic)


set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)