cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project(
  QST
  VERSION 1.6.7
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

# Boost
find_package(Boost 1.67.0 REQUIRED COMPONENTS iostreams filesystem)
include_directories(${BOOST_INCLUDE_DIRS})

# Add src files
set(SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/main.cpp ${PROJECT_SOURCE_DIR}/src/qsort_c.c
    ${PROJECT_SOURCE_DIR}/src/sort.c)

set(HEADER_FILES
    ${PROJECT_SOURCE_DIR}/src/exp.hpp ${PROJECT_SOURCE_DIR}/src/io.hpp
    ${PROJECT_SOURCE_DIR}/src/platform.hpp ${PROJECT_SOURCE_DIR}/src/sort.h
    ${PROJECT_SOURCE_DIR}/src/sort.hpp)

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

add_compile_options(
  -Wall
  -Wextra
  -Wconversion
  -Wsign-conversion
  -pedantic
  $<$<CONFIG:DEBUG>:-g3>
  $<$<CONFIG:DEBUG>:-Og>
  $<$<CONFIG:DEBUG>:-pg>
  $<$<CONFIG:DEBUG>:-p>
  $<$<CONFIG:RELEASE>:-O2>)

# Docs
find_package(Doxygen)
if(${DOXYGEN_FOUND} AND ${DOXYGEN_VERSION} VERSION_GREATER "1.9.0")
  set(DOXYGEN_INPUT_DIR
      ${PROJECT_SOURCE_DIR}/src,${PROJECT_SOURCE_DIR}/evaluator)
  set(DOXYGEN_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/docs/doxygen)
  set(DOXYGEN_INDEX_FILE ${CMAKE_CURRENT_SOURCE_DIR}/docs/html/index.html)
  set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
  set(DOXYFILE_OUT ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile)

  configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

  file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
  add_custom_command(
    OUTPUT ${DOXYGEN_INDEX_FILE}
    DEPENDS ${HEADER_FILES}
    DEPENDS ${SRC_FILES}
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
    COMMENT "Generating docs")
  add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})
endif()
