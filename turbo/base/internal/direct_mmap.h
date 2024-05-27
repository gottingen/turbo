// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// Functions for directly invoking mmap() via syscall, avoiding the case where
// mmap() has been locally overridden.

#ifndef TURBO_BASE_INTERNAL_DIRECT_MMAP_H_
#define TURBO_BASE_INTERNAL_DIRECT_MMAP_H_

#include <turbo/base/config.h>

#ifdef TURBO_HAVE_MMAP

#include <sys/mman.h>

#ifdef __linux__

#include <sys/types.h>
#ifdef __BIONIC__
#include <sys/syscall.h>
#else
#include <syscall.h>
#endif

#include <linux/unistd.h>
#include <unistd.h>
#include <cerrno>
#include <cstdarg>
#include <cstdint>

#ifdef __mips__
// Include definitions of the ABI currently in use.
#if defined(__BIONIC__) || !defined(__GLIBC__)
// Android doesn't have sgidefs.h, but does have asm/sgidefs.h, which has the
// definitions we need.
#include <asm/sgidefs.h>
#else
#include <sgidefs.h>
#endif  // __BIONIC__ || !__GLIBC__
#endif  // __mips__

// SYS_mmap and SYS_munmap are not defined in Android.
#ifdef __BIONIC__
extern "C" void* __mmap2(void*, size_t, int, int, int, size_t);
#if defined(__NR_mmap) && !defined(SYS_mmap)
#define SYS_mmap __NR_mmap
#endif
#ifndef SYS_munmap
#define SYS_munmap __NR_munmap
#endif
#endif  // __BIONIC__

#if defined(__NR_mmap2) && !defined(SYS_mmap2)
#define SYS_mmap2 __NR_mmap2
#endif

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

// Platform specific logic extracted from
// https://chromium.googlesource.com/linux-syscall-support/+/master/linux_syscall_support.h
inline void* DirectMmap(void* start, size_t length, int prot, int flags, int fd,
                        off_t offset) noexcept {
#if defined(__i386__) || defined(__ARM_ARCH_3__) || defined(__ARM_EABI__) || \
    defined(__m68k__) || defined(__sh__) ||                                  \
    (defined(__hppa__) && !defined(__LP64__)) ||                             \
    (defined(__mips__) && _MIPS_SIM == _MIPS_SIM_ABI32) ||                   \
    (defined(__PPC__) && !defined(__PPC64__)) ||                             \
    (defined(__riscv) && __riscv_xlen == 32) ||                              \
    (defined(__s390__) && !defined(__s390x__)) ||                            \
    (defined(__sparc__) && !defined(__arch64__))
  // On these architectures, implement mmap with mmap2.
  static int pagesize = 0;
  if (pagesize == 0) {
#if defined(__wasm__) || defined(__asmjs__)
    pagesize = getpagesize();
#else
    pagesize = sysconf(_SC_PAGESIZE);
#endif
  }
  if (offset < 0 || offset % pagesize != 0) {
    errno = EINVAL;
    return MAP_FAILED;
  }
#ifdef __BIONIC__
  // SYS_mmap2 has problems on Android API level <= 16.
  // Workaround by invoking __mmap2() instead.
  return __mmap2(start, length, prot, flags, fd,
                 static_cast<size_t>(offset / pagesize));
#else
  return reinterpret_cast<void*>(
      syscall(SYS_mmap2, start, length, prot, flags, fd,
              static_cast<unsigned long>(offset / pagesize)));  // NOLINT
#endif
#elif defined(__s390x__)
  // On s390x, mmap() arguments are passed in memory.
  unsigned long buf[6] = {reinterpret_cast<unsigned long>(start),  // NOLINT
                          static_cast<unsigned long>(length),      // NOLINT
                          static_cast<unsigned long>(prot),        // NOLINT
                          static_cast<unsigned long>(flags),       // NOLINT
                          static_cast<unsigned long>(fd),          // NOLINT
                          static_cast<unsigned long>(offset)};     // NOLINT
  return reinterpret_cast<void*>(syscall(SYS_mmap, buf));
#elif defined(__x86_64__)
// The x32 ABI has 32 bit longs, but the syscall interface is 64 bit.
// We need to explicitly cast to an unsigned 64 bit type to avoid implicit
// sign extension.  We can't cast pointers directly because those are
// 32 bits, and gcc will dump ugly warnings about casting from a pointer
// to an integer of a different size. We also need to make sure __off64_t
// isn't truncated to 32-bits under x32.
#define MMAP_SYSCALL_ARG(x) ((uint64_t)(uintptr_t)(x))
  return reinterpret_cast<void*>(
      syscall(SYS_mmap, MMAP_SYSCALL_ARG(start), MMAP_SYSCALL_ARG(length),
              MMAP_SYSCALL_ARG(prot), MMAP_SYSCALL_ARG(flags),
              MMAP_SYSCALL_ARG(fd), static_cast<uint64_t>(offset)));
#undef MMAP_SYSCALL_ARG
#else  // Remaining 64-bit aritectures.
  static_assert(sizeof(unsigned long) == 8, "Platform is not 64-bit");
  return reinterpret_cast<void*>(
      syscall(SYS_mmap, start, length, prot, flags, fd, offset));
#endif
}

inline int DirectMunmap(void* start, size_t length) {
  return static_cast<int>(syscall(SYS_munmap, start, length));
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#else  // !__linux__

// For non-linux platforms where we have mmap, just dispatch directly to the
// actual mmap()/munmap() methods.

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

inline void* DirectMmap(void* start, size_t length, int prot, int flags, int fd,
                        off_t offset) {
  return mmap(start, length, prot, flags, fd, offset);
}

inline int DirectMunmap(void* start, size_t length) {
  return munmap(start, length);
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // __linux__

#endif  // TURBO_HAVE_MMAP

#endif  // TURBO_BASE_INTERNAL_DIRECT_MMAP_H_
