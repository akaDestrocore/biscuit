# Set the minimum version of CMake that can be used
cmake_minimum_required(VERSION 3.20)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

# Set target definition variables required by the toolchain file
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake)

# Set the toolchain file for cross-compilation
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake)

# Project definition
project(genx_400hf_l)

# Enable CMake support for ASM and C/CPP languages
enable_language(C ASM)

set(PROJECT_NAME genx_400hf_l)

# Create an executable object type
add_executable(${PROJECT_NAME})

# Include additional CMake files for flags, components, and files
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/flags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/components.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/files.cmake)

# Add sources to executable/library
target_sources(${PROJECT_NAME} PRIVATE
  # User defined sources
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/core.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/nextion.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/alarm.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/bipolar.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/error.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/input.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/monopolar.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/plate.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/selftest.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/touch.c"
)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
  # User defined include directories
)

# Compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
  # User defined compiler options
)

# Add additional defined symbols
target_compile_definitions(${PROJECT_NAME} PRIVATE
  # Add additional defined symbols here
  STM32F407xx
)

# Linker options
target_link_options(${PROJECT_NAME} PRIVATE 
  # User defined linker options
)

# Add additional linker search directories
target_link_directories(${PROJECT_NAME} PRIVATE
  # Add additional linker search directories here
)

# Add additional linker libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
  # Add additional linker libraries here
)

find_program(STM32_PROGRAMMER_CLI STM32_Programmer_CLI)
if(NOT STM32_PROGRAMMER_CLI)
    message(WARNING "STM32_Programmer_CLI not found on PATH")
endif()

add_custom_target(flash
    COMMAND ${STM32_PROGRAMMER_CLI} -c port=SWD -w $<TARGET_FILE:${CMAKE_PROJECT_NAME}> -v -rst
    DEPENDS ${CMAKE_PROJECT_NAME}
    COMMENT "Flashing ${CMAKE_PROJECT_NAME}.elf"
)