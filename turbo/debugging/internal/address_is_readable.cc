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

// base::AddressIsReadable() probes an address to see whether it is readable,
// without faulting.

#include <turbo/debugging/internal/address_is_readable.h>

#if !defined(__linux__) || defined(__ANDROID__)

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

// On platforms other than Linux, just return true.
bool AddressIsReadable(const void* /* addr */) { return true; }

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#else  // __linux__ && !__ANDROID__

#include <stdint.h>
#include <syscall.h>
#include <unistd.h>

#include <turbo/base/internal/errno_saver.h>
#include <turbo/base/internal/raw_logging.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

// NOTE: be extra careful about adding any interposable function calls here
// (such as open(), read(), etc.). These symbols may be interposed and will get
// invoked in contexts they don't expect.
//
// NOTE: any new system calls here may also require sandbox reconfiguration.
//
bool AddressIsReadable(const void *addr) {
  // rt_sigprocmask below checks 8 contiguous bytes. If addr resides in the
  // last 7 bytes of a page (unaligned), rt_sigprocmask would additionally
  // check the readability of the next page, which is not desired. Align
  // address on 8-byte boundary to check only the current page.
  const uintptr_t u_addr = reinterpret_cast<uintptr_t>(addr) & ~uintptr_t{7};
  addr = reinterpret_cast<const void *>(u_addr);

  // rt_sigprocmask below will succeed for this input.
  if (addr == nullptr) return false;

  turbo::base_internal::ErrnoSaver errno_saver;

  // Here we probe with some syscall which
  // - accepts an 8-byte region of user memory as input
  // - tests for EFAULT before other validation
  // - has no problematic side-effects
  //
  // rt_sigprocmask(2) works for this.  It copies sizeof(kernel_sigset_t)==8
  // bytes from the address into the kernel memory before any validation.
  //
  // The call can never succeed, since the `how` parameter is not one of
  // SIG_BLOCK, SIG_UNBLOCK, SIG_SETMASK.
  //
  // This strategy depends on Linux implementation details,
  // so we rely on the test to alert us if it stops working.
  //
  // Some discarded past approaches:
  // - msync() doesn't reject PROT_NONE regions
  // - write() on /dev/null doesn't return EFAULT
  // - write() on a pipe requires creating it and draining the writes
  // - connect() works but is problematic for sandboxes and needs a valid
  //   file descriptor
  //
  // This can never succeed (invalid first argument to sigprocmask).
  TURBO_RAW_CHECK(syscall(SYS_rt_sigprocmask, ~0, addr, nullptr,
                         /*sizeof(kernel_sigset_t)*/ 8) == -1,
                 "unexpected success");
  TURBO_RAW_CHECK(errno == EFAULT || errno == EINVAL, "unexpected errno");
  return errno != EFAULT;
}

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // __linux__ && !__ANDROID__
