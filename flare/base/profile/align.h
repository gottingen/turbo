
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_ALIGN_H
#define FLARE_ALIGN_H

#include <cstddef>

namespace flare {
    constexpr std::size_t max_align_v = alignof(max_align_t);

    // At the time of writing, GCC 8.2 has not implemented these constants.
#if defined(__x86_64__)

    // On Sandy Bridge, accessing adjacent cache lines also see destructive
    // interference.
    //
    // @sa: https://github.com/facebook/folly/blob/master/folly/lang/Align.h
    //
    // Update at 20201124: Well, AMD's Zen 3 does the same.
    constexpr std::size_t hardware_destructive_interference_size = 128;
    constexpr std::size_t hardware_constructive_interference_size = 64;

#elif defined(__aarch64__)

    // AArch64 is ... weird, to say the least. Some vender (notably Samsung) uses a
// non-consistent cacheline size across BIG / little cores..
//
// Let's ignore those CPUs for now.
//
// @sa: https://www.mono-project.com/news/2016/09/12/arm64-icache/
constexpr std::size_t hardware_destructive_interference_size = 64;
constexpr std::size_t hardware_constructive_interference_size = 64;

#elif defined(__powerpc64__)

// These values are read from
// `/sys/devices/system/cpu/cpu0/cache/index*/coherency_line_size`
constexpr std::size_t hardware_destructive_interference_size = 128;
constexpr std::size_t hardware_constructive_interference_size = 128;

#else

#error Unsupported architecture.

#endif

}  // namespace flare

#endif //FLARE_ALIGN_H
