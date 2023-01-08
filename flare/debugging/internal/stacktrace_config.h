
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_DEBUGGING_INTERNAL_STACKTRACE_CONFIG_H_
#define FLARE_DEBUGGING_INTERNAL_STACKTRACE_CONFIG_H_

#if defined(FLARE_STACKTRACE_INL_HEADER)
#error FLARE_STACKTRACE_INL_HEADER cannot be directly set

#elif defined(_WIN32)
#define FLARE_STACKTRACE_INL_HEADER \
    "flare/debugging/internal/stacktrace_win32-inl.inc"

#elif defined(__linux__) && !defined(__ANDROID__)

#if !defined(NO_FRAME_POINTER)
# if defined(__i386__) || defined(__x86_64__)
#define FLARE_STACKTRACE_INL_HEADER \
    "flare/debugging/internal/stacktrace_x86-inl.inc"
# elif defined(__ppc__) || defined(__PPC__)
#define FLARE_STACKTRACE_INL_HEADER \
    "flare/debugging/internal/stacktrace_powerpc-inl.inc"
# elif defined(__aarch64__)
#define FLARE_STACKTRACE_INL_HEADER \
    "flare/debugging/internal/stacktrace_aarch64-inl.inc"
# elif defined(__arm__)
// Note: When using glibc this may require -funwind-tables to function properly.
#define FLARE_STACKTRACE_INL_HEADER \
  "flare/debugging/internal/stacktrace_generic-inl.inc"
# else
#define FLARE_STACKTRACE_INL_HEADER \
   "flare/debugging/internal/stacktrace_unimplemented-inl.inc"
# endif
#else  // defined(NO_FRAME_POINTER)
# if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__)
#define FLARE_STACKTRACE_INL_HEADER \
    "flare/debugging/internal/stacktrace_generic-inl.inc"
# elif defined(__ppc__) || defined(__PPC__)
#define FLARE_STACKTRACE_INL_HEADER \
    "flare/debugging/internal/stacktrace_generic-inl.inc"
# else
#define FLARE_STACKTRACE_INL_HEADER \
   "flare/debugging/internal/stacktrace_unimplemented-inl.inc"
# endif
#endif  // NO_FRAME_POINTER

#else
#define FLARE_STACKTRACE_INL_HEADER \
  "flare/debugging/internal/stacktrace_unimplemented-inl.inc"

#endif

#endif  // FLARE_DEBUGGING_INTERNAL_STACKTRACE_CONFIG_H_
