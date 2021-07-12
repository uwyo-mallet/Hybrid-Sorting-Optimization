cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(
  QST
  VERSION 1.0.2
  DESCRIPTION "Optimizing Quicksort"
  LANGUAGES CXX)
configure_file(src/config.h.in config.hpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message("CXX Compiler Path: " ${CMAKE_CXX_COMPILER})
message("CXX Compiler ID: " ${CMAKE_CXX_COMPILER_ID})

# Add src files
set(SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/io.cpp ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/sort.cpp)

set(EXECUTABLE_NAME QST)
add_executable(${EXECUTABLE_NAME} ${SRC_FILES})

# Include for config
target_include_directories(${EXECUTABLE_NAME} PRIVATE "${PROJECT_BINARY_DIR}")

# Compiler warnings, platform specific.
add_compile_options("$<$<CONFIG:RELEASE>:-W;-Wall;-O3;-pedantic>")
add_compile_options("$<$<CONFIG:DEBUG>:-W;-Wall;-O0;-g;-pedantic>")
