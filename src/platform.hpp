#ifndef PLATFORM_HPP_
#define PLATFORM_HPP_

// Counting CPU Cycles
#include <x86intrin.h>

// Default features are disabled
#define ASM_ENABLED "-"

#ifdef USE_BOOST_CPP_INT
#define BOOST_CPP_INT "+"
namespace bmp = boost::multiprecision;
#else
#define BOOST_CPP_INT "-"
#endif  // USE_BOOST_CPP_INT

#if MSVC
#error "Unsupported compiler"
#endif  // MSVC

#ifdef __linux__
#if defined(__x86_64__) && !defined(USE_BOOST_CPP_INT) && !defined(DISABLE_ASM)
#define ARCH_X86
#undef ASM_ENABLED
#define ASM_ENABLED "+"
#endif  // __x86_64__ && !USE_BOOST_CPP_INT
#endif  // __linux__

#endif  // PLATFORM_HPP_
