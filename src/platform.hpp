#ifndef PLATFORM_HPP_
#define PLATFORM_HPP_

// Default features are disabled
#define ASM_ENABLED "-"

#ifdef MSVC
#error "Unsupported compiler"
#endif  // MSVC

#ifdef __linux__
#if defined(__x86_64__) && !defined(DISABLE_ASM)
#define ARCH_X86
#undef ASM_ENABLED
#define ASM_ENABLED "+"
#endif  // __x86_64__ && !USE_BOOST_CPP_INT
#endif  // __linux__

#endif  // PLATFORM_HPP_
