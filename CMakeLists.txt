cmake_minimum_required(VERSION 3.20)
project(Nes_Emu C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # for clangd

# core
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/core/*.c"
        "${CMAKE_SOURCE_DIR}/src/core/mappers/*.c")

add_library(nes_core STATIC ${CORE_SOURCES})
target_include_directories(nes_core PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_compile_options(nes_core PRIVATE -Wall -Wextra -Wpedantic)

# front
file(GLOB FRONTEND_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/frontend/*.c")

if(APPLE)
    execute_process(COMMAND brew --prefix
            OUTPUT_VARIABLE HOMEBREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    if(HOMEBREW_PREFIX)
        list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}")
    endif()
endif()

find_package(SDL2 REQUIRED)

add_executable(nesemu ${FRONTEND_SOURCES})
target_link_libraries(nesemu PRIVATE nes_core SDL2::SDL2)
target_compile_options(nesemu PRIVATE -Wall -Wextra)

# tests
# Not linked to SDL
add_executable(nestest_runner tests/nestest_runner.c)
target_link_libraries(nestest_runner PRIVATE nes_core)

enable_testing()
add_test(NAME nestest
        COMMAND nestest_runner "${CMAKE_SOURCE_DIR}/tests/roms/nestest.nes")