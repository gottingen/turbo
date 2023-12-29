/*

    libcontext - a slightly more portable version of boost::context

    Copyright Martin Husemann 2013.
    Copyright Oliver Kowalke 2009.
    Copyright Sergue E. Leontiev 2013.
    Copyright Thomas Sailer 2013.
    Minor modifications by Tomasz Wlostowski 2016.

 Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)

*/

#ifndef TURBO_FIBER_CONTEXT_H_
#define TURBO_FIBER_CONTEXT_H_

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#if defined(__GNUC__) || defined(__APPLE__)

#define FIBER_CONTEXT_COMPILER_gcc

#if defined(__linux__)
#ifdef __x86_64__
#define FIBER_CONTEXT_PLATFORM_linux_x86_64
#define FIBER_CONTEXT_CALL_CONVENTION

#elif __i386__
#define FIBER_CONTEXT_PLATFORM_linux_i386
#define FIBER_CONTEXT_CALL_CONVENTION
#elif __arm__
#define FIBER_CONTEXT_PLATFORM_linux_arm32
#define FIBER_CONTEXT_CALL_CONVENTION
#elif __aarch64__
#define FIBER_CONTEXT_PLATFORM_linux_arm64
#define FIBER_CONTEXT_CALL_CONVENTION
#elif __loongarch64
#define FIBER_CONTEXT_PLATFORM_linux_loongarch64
#define FIBER_CONTEXT_CALL_CONVENTION
#endif

#elif defined(__MINGW32__) || defined (__MINGW64__)
#if defined(__x86_64__)
#define FIBER_CONTEXT_COMPILER_gcc
#define FIBER_CONTEXT_PLATFORM_windows_x86_64
#define FIBER_CONTEXT_CALL_CONVENTION
#elif defined(__i386__)
#define FIBER_CONTEXT_COMPILER_gcc
#define FIBER_CONTEXT_PLATFORM_windows_i386
#define FIBER_CONTEXT_CALL_CONVENTION __cdecl
#endif

#elif defined(__APPLE__) && defined(__MACH__)
#if defined (__i386__)
#define FIBER_CONTEXT_PLATFORM_apple_i386
#define FIBER_CONTEXT_CALL_CONVENTION
#elif defined (__x86_64__)
#define FIBER_CONTEXT_PLATFORM_apple_x86_64
#define FIBER_CONTEXT_CALL_CONVENTION
#elif defined (__aarch64__)
#define FIBER_CONTEXT_PLATFORM_apple_arm64
#define FIBER_CONTEXT_CALL_CONVENTION
#endif
#endif

#endif

#if defined(_WIN32_WCE)
typedef int intptr_t;
#endif

typedef void *fiber_fcontext_t;

#ifdef __cplusplus
extern "C" {
#endif

intptr_t FIBER_CONTEXT_CALL_CONVENTION
fiber_jump_fcontext(fiber_fcontext_t *ofc, fiber_fcontext_t nfc,
                    intptr_t vp, bool preserve_fpu = false);
fiber_fcontext_t FIBER_CONTEXT_CALL_CONVENTION
fiber_make_fcontext(void *sp, size_t size, void (*fn)(intptr_t));

#ifdef __cplusplus
};
#endif

#endif  // TURBO_FIBER_CONTEXT_H_
