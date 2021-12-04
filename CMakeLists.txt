cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(
  QST
  VERSION 1.8.4
  DESCRIPTION "Optimizing Quicksort"
  LANGUAGES C CXX ASM)
configure_file(src/config.h.in config.hpp)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message("C Compiler Path: " ${CMAKE_C_COMPILER})
message("C Compiler ID: " ${CMAKE_C_COMPILER_ID})

message("CXX Compiler Path: " ${CMAKE_CXX_COMPILER})
message("CXX Compiler ID: " ${CMAKE_CXX_COMPILER_ID})

# Additional modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Boost
find_package(Boost 1.67.0 REQUIRED COMPONENTS iostreams filesystem timer)
include_directories(${BOOST_INCLUDE_DIRS})

# Add src files
set(SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/main.cpp ${PROJECT_SOURCE_DIR}/src/qsort_c.c
    ${PROJECT_SOURCE_DIR}/src/sort.c ${PROJECT_SOURCE_DIR}/src/utils.cpp)

set(HEADER_FILES
    ${PROJECT_SOURCE_DIR}/src/exp.hpp ${PROJECT_SOURCE_DIR}/src/io.hpp
    ${PROJECT_SOURCE_DIR}/src/platform.hpp ${PROJECT_SOURCE_DIR}/src/sort.h
    ${PROJECT_SOURCE_DIR}/src/sort.hpp ${PROJECT_SOURCE_DIR}/src/utils.hpp)

if(USE_BOOST_CPP_INT)
  add_definitions(-DUSE_BOOST_CPP_INT)
  set(DISABLE_ASM ON)
endif(USE_BOOST_CPP_INT)

if(DISABLE_ASM)
  add_definitions(-DDISABLE_ASM)
else()
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
    list(APPEND SRC_FILES ${PROJECT_SOURCE_DIR}/src/ins_sort.s)
    list(APPEND SRC_FILES ${PROJECT_SOURCE_DIR}/src/qsort_asm.s)
  endif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
endif(DISABLE_ASM)

set(EXECUTABLE_NAME QST)
add_executable(${EXECUTABLE_NAME} ${SRC_FILES})

# Include for config
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${PROJECT_BINARY_DIR})

# Linking
target_link_libraries(${EXECUTABLE_NAME} Boost::iostreams Boost::filesystem
                      Boost::timer)

target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra -Wconversion
                                                  -Wsign-conversion -pedantic)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  message("Build type: Debug")
  target_compile_options(${EXECUTABLE_NAME} PRIVATE -ggdb -fsanitize=address)
elseif(CMAKE_BUILD_TYPE MATCHES "Valgrind")
  message("Build type: Valgrind")
  target_compile_options(${EXECUTABLE_NAME} PRIVATE -Ofast -ggdb)
else()
  message("Build type: Release")
  target_compile_options(${EXECUTABLE_NAME} PRIVATE -Ofast)
endif()

# Docs
add_subdirectory("docs")
