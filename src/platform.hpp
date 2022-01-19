#ifndef PLATFORM_HPP_
#define PLATFORM_HPP_

#include <cstddef>

// Default features are disabled
#define ASM_ENABLED "-"

#ifdef MSVC
#error "Unsupported compiler"
#endif  // MSVC

#ifndef __linux__
#error "Only Linux is supported"
#endif  // __linux__

#if defined(__x86_64__) && !defined(DISABLE_ASM)
#define ARCH_X86
#undef ASM_ENABLED
#define ASM_ENABLED "+"
#endif  // __x86_64__ && !USE_BOOST_CPP_INT

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
#define USING_GLIBCXX "+"
#else
#define USING_GLIBCXX "-"
#endif  // __GLIBCXX__ || __GLIBCPP__

// clang-format off
#define VERSION(VERSION_NUM)                                           \
   VERSION_NUM                                                         \
  "\n\tC COMPILER: " C_COMPILER_ID " " C_COMPILER_VERSION              \
  "\n\tCXX COMPILER: " CXX_COMPILER_ID " " CXX_COMPILER_VERSION        \
  "\n\tType: " CMAKE_BUILD_TYPE                                        \
  "\n\tASM Methods: [" ASM_ENABLED "]"                                 \
  "\n\tUsing GLIBCXX: [" USING_GLIBCXX "]"
// clang-format on

#endif  // PLATFORM_HPP_
