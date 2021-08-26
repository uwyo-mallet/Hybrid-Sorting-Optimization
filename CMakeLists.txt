cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(
  QST
  VERSION 1.5.3
  DESCRIPTION "Optimizing Quicksort"
  LANGUAGES CXX ASM)
configure_file(src/config.h.in config.hpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message("CXX Compiler Path: " ${CMAKE_CXX_COMPILER})
message("CXX Compiler ID: " ${CMAKE_CXX_COMPILER_ID})

# Boost
find_package(Boost 1.67.0 REQUIRED COMPONENTS iostreams filesystem)
include_directories(${BOOST_INCLUDE_DIRS})

# Add src files
set(SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp
              ${PROJECT_SOURCE_DIR}/src/qsort_c.cpp)

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
                      Boost::boost)

# Compiler warnings, platform specific.
add_compile_options("$<$<CONFIG:RELEASE>:-W;-Wall;-O2;-pedantic>")
add_compile_options("$<$<CONFIG:DEBUG>:-W;-Wall;-O0;-g;-pedantic>")
