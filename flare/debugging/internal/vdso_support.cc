
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

// Allow dynamic symbol lookup in the kernel VDSO page.
//
// VDSOSupport -- a class representing kernel VDSO (if present).

#include "flare/debugging/internal/vdso_support.h"

#ifdef FLARE_HAVE_VDSO_SUPPORT     // defined in vdso_support.h

#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <unistd.h>

#if __GLIBC_PREREQ(2, 16)  // GLIBC-2.16 implements getauxval.
#include <sys/auxv.h>
#endif

#include "flare/base/dynamic_annotations/dynamic_annotations.h"
#include "flare/log/logging.h"
#include "flare/base/profile.h"

#ifndef AT_SYSINFO_EHDR
#define AT_SYSINFO_EHDR 33  // for crosstoolv10
#endif

namespace flare::debugging {

namespace debugging_internal {

std::atomic<const void *> VDSOSupport::vdso_base_(
    debugging_internal::ElfMemImage::kInvalidBase);

std::atomic<VDSOSupport::GetCpuFn> VDSOSupport::getcpu_fn_(&InitAndGetCPU);
VDSOSupport::VDSOSupport()
    // If vdso_base_ is still set to kInvalidBase, we got here
    // before VDSOSupport::Init has been called. Call it now.
    : image_(vdso_base_.load(std::memory_order_relaxed) ==
                     debugging_internal::ElfMemImage::kInvalidBase
                 ? Init()
                 : vdso_base_.load(std::memory_order_relaxed)) {}

// NOTE: we can't use GoogleOnceInit() below, because we can be
// called by tcmalloc, and none of the *once* stuff may be functional yet.
//
// In addition, we hope that the VDSOSupportHelper constructor
// causes this code to run before there are any threads, and before
// InitGoogle() has executed any chroot or setuid calls.
//
// Finally, even if there is a race here, it is harmless, because
// the operation should be idempotent.
const void *VDSOSupport::Init() {
  const auto kInvalidBase = debugging_internal::ElfMemImage::kInvalidBase;
#if __GLIBC_PREREQ(2, 16)
  if (vdso_base_.load(std::memory_order_relaxed) == kInvalidBase) {
    errno = 0;
    const void *const sysinfo_ehdr =
        reinterpret_cast<const void *>(getauxval(AT_SYSINFO_EHDR));
    if (errno == 0) {
      vdso_base_.store(sysinfo_ehdr, std::memory_order_relaxed);
    }
  }
#endif  // __GLIBC_PREREQ(2, 16)
  if (vdso_base_.load(std::memory_order_relaxed) == kInvalidBase) {
    // Valgrind zaps AT_SYSINFO_EHDR and friends from the auxv[]
    // on stack, and so glibc works as if VDSO was not present.
    // But going directly to kernel via /proc/self/auxv below bypasses
    // Valgrind zapping. So we check for Valgrind separately.
    if (RunningOnValgrind()) {
      vdso_base_.store(nullptr, std::memory_order_relaxed);
      getcpu_fn_.store(&GetCPUViaSyscall, std::memory_order_relaxed);
      return nullptr;
    }
    int fd = open("/proc/self/auxv", O_RDONLY);
    if (fd == -1) {
      // Kernel too old to have a VDSO.
      vdso_base_.store(nullptr, std::memory_order_relaxed);
      getcpu_fn_.store(&GetCPUViaSyscall, std::memory_order_relaxed);
      return nullptr;
    }
    ElfW(auxv_t) aux;
    while (read(fd, &aux, sizeof(aux)) == sizeof(aux)) {
      if (aux.a_type == AT_SYSINFO_EHDR) {
        vdso_base_.store(reinterpret_cast<void *>(aux.a_un.a_val),
                         std::memory_order_relaxed);
        break;
      }
    }
    close(fd);
    if (vdso_base_.load(std::memory_order_relaxed) == kInvalidBase) {
      // Didn't find AT_SYSINFO_EHDR in auxv[].
      vdso_base_.store(nullptr, std::memory_order_relaxed);
    }
  }
  GetCpuFn fn = &GetCPUViaSyscall;  // default if VDSO not present.
  if (vdso_base_.load(std::memory_order_relaxed)) {
    VDSOSupport vdso;
    SymbolInfo info;
    if (vdso.LookupSymbol("__vdso_getcpu", "LINUX_2.6", STT_FUNC, &info)) {
      fn = reinterpret_cast<GetCpuFn>(const_cast<void *>(info.address));
    }
  }
  // Subtle: this code runs outside of any locks; prevent compiler
  // from assigning to getcpu_fn_ more than once.
  getcpu_fn_.store(fn, std::memory_order_relaxed);
  return vdso_base_.load(std::memory_order_relaxed);
}

const void *VDSOSupport::SetBase(const void *base) {
  FLARE_CHECK(base != debugging_internal::ElfMemImage::kInvalidBase)<<"internal error";
  const void *old_base = vdso_base_.load(std::memory_order_relaxed);
  vdso_base_.store(base, std::memory_order_relaxed);
  image_.Init(base);
  // Also reset getcpu_fn_, so GetCPU could be tested with simulated VDSO.
  getcpu_fn_.store(&InitAndGetCPU, std::memory_order_relaxed);
  return old_base;
}

bool VDSOSupport::LookupSymbol(const char *name,
                               const char *version,
                               int type,
                               SymbolInfo *info) const {
  return image_.LookupSymbol(name, version, type, info);
}

bool VDSOSupport::LookupSymbolByAddress(const void *address,
                                        SymbolInfo *info_out) const {
  return image_.LookupSymbolByAddress(address, info_out);
}

// NOLINT on 'long' because this routine mimics kernel api.
long VDSOSupport::GetCPUViaSyscall(unsigned *cpu,  // NOLINT(runtime/int)
                                   void *, void *) {
#ifdef SYS_getcpu
  return syscall(SYS_getcpu, cpu, nullptr, nullptr);
#else
  // x86_64 never implemented sys_getcpu(), except as a VDSO call.
  static_cast<void>(cpu);  // Avoid an unused argument compiler warning.
  errno = ENOSYS;
  return -1;
#endif
}

// Use fast __vdso_getcpu if available.
long VDSOSupport::InitAndGetCPU(unsigned *cpu,  // NOLINT(runtime/int)
                                void *x, void *y) {
  Init();
  GetCpuFn fn = getcpu_fn_.load(std::memory_order_relaxed);
  FLARE_CHECK(fn != &InitAndGetCPU)<< "Init() did not set getcpu_fn_";
  return (*fn)(cpu, x, y);
}

// This function must be very fast, and may be called from very
// low level (e.g. tcmalloc). Hence I avoid things like
// GoogleOnceInit() and ::operator new.
FLARE_NO_SANITIZE_MEMORY
int GetCPU() {
  unsigned cpu;
  int ret_code = (*VDSOSupport::getcpu_fn_)(&cpu, nullptr, nullptr);
  return ret_code == 0 ? cpu : ret_code;
}

// We need to make sure VDSOSupport::Init() is called before
// InitGoogle() does any setuid or chroot calls.  If VDSOSupport
// is used in any global constructor, this will happen, since
// VDSOSupport's constructor calls Init.  But if not, we need to
// ensure it here, with a global constructor of our own.  This
// is an allowed exception to the normal rule against non-trivial
// global constructors.
static class VDSOInitHelper {
 public:
  VDSOInitHelper() { VDSOSupport::Init(); }
} vdso_init_helper;

}  // namespace debugging_internal

}  // namespace flare::debugging

#endif  // FLARE_HAVE_VDSO_SUPPORT
