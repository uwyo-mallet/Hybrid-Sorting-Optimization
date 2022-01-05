cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(
  QST
  VERSION 2.0.2
  DESCRIPTION "Optimizing Quicksort"
  LANGUAGES C CXX ASM)
configure_file(src/config.h.in config.hpp)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CXX_STANDARD 14)
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

# Google Benchmark
find_package(benchmark)
if(benchmark_FOUND)
  # pthreads
  find_package(Threads REQUIRED)

  set(GB_QST_SRC_FILES
      ${PROJECT_SOURCE_DIR}/src/gb/gb_main.cpp
      ${PROJECT_SOURCE_DIR}/src/gb/benchmark.cpp
      ${PROJECT_SOURCE_DIR}/src/qsort_c.c
      ${PROJECT_SOURCE_DIR}/src/sort.c
      ${PROJECT_SOURCE_DIR}/src/utils.cpp)
  add_executable(GB_QST ${GB_QST_SRC_FILES})

  # Include
  target_include_directories(GB_QST PRIVATE ${PROJECT_SOURCE_DIR}/src
                                            ${PROJECT_SOURCE_DIR}/src/gb)
  target_include_directories(GB_QST PRIVATE ${PROJECT_BINARY_DIR})

  # Linking
  target_link_libraries(GB_QST Boost::iostreams Boost::filesystem
                        Threads::Threads benchmark::benchmark)

  # Build Types
  if(CMAKE_BUILD_TYPE MATCHES "Debug")
    # Address sanitizer
    set(SANITIZER_OPTS -fsanitize=address)
    target_compile_options(GB_QST PRIVATE -Og -ggdb ${SANITIZER_OPTS})
    target_link_options(GB_QST PRIVATE ${SANITIZER_OPTS})
  elseif(CMAKE_BUILD_TYPE MATCHES "Valgrind")
    target_compile_options(GB_QST PRIVATE -O2 -ggdb)
  else()
    target_compile_options(GB_QST PRIVATE -O2)
  endif()
else()
  message("Google Benchmark not found, not building GB_QST")
endif()

# Add src files
set(QST_SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/main.cpp ${PROJECT_SOURCE_DIR}/src/qsort_c.c
    ${PROJECT_SOURCE_DIR}/src/sort.c ${PROJECT_SOURCE_DIR}/src/utils.cpp)
if(DISABLE_ASM)
  add_definitions(-DDISABLE_ASM)
else()
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
    list(APPEND QST_SRC_FILES ${PROJECT_SOURCE_DIR}/src/ins_sort.s)
    list(APPEND QST_SRC_FILES ${PROJECT_SOURCE_DIR}/src/qsort_asm.s)
  endif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
endif(DISABLE_ASM)

add_executable(QST ${QST_SRC_FILES})

# Include for config
target_include_directories(QST PRIVATE ${PROJECT_BINARY_DIR})

# Linking
target_link_libraries(QST Boost::iostreams Boost::filesystem Boost::timer)
target_compile_options(QST PRIVATE -Wall -Wextra -Wconversion -Wsign-conversion
                                   -pedantic)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  message("Build type: Debug")
  set(SANITIZER_OPTS -fsanitize=address)
  target_compile_options(QST PRIVATE -Og -ggdb ${SANITIZER_OPTS})
  target_link_options(QST PRIVATE ${SANITIZER_OPTS})
elseif(CMAKE_BUILD_TYPE MATCHES "Valgrind")
  message("Build type: Valgrind")
  target_compile_options(QST PRIVATE -O2 -ggdb)
else()
  message("Build type: Release")
  target_compile_options(QST PRIVATE -O2)
endif()

# Docs
add_subdirectory("docs")
