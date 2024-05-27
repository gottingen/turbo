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
// Wrappers around lsan_interface functions.
//
// These are always-available run-time functions manipulating the LeakSanitizer,
// even when the lsan_interface (and LeakSanitizer) is not available. When
// LeakSanitizer is not linked in, these functions become no-op stubs.

#include <turbo/debugging/leak_check.h>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>

#if defined(TURBO_HAVE_LEAK_SANITIZER)

#include <sanitizer/lsan_interface.h>

#if TURBO_HAVE_ATTRIBUTE_WEAK
extern "C" TURBO_ATTRIBUTE_WEAK int __lsan_is_turned_off();
#endif

namespace turbo {
TURBO_NAMESPACE_BEGIN
bool HaveLeakSanitizer() { return true; }

#if TURBO_HAVE_ATTRIBUTE_WEAK
bool LeakCheckerIsActive() {
  return !(&__lsan_is_turned_off && __lsan_is_turned_off());
}
#else
bool LeakCheckerIsActive() { return true; }
#endif

bool FindAndReportLeaks() { return __lsan_do_recoverable_leak_check(); }
void DoIgnoreLeak(const void* ptr) { __lsan_ignore_object(ptr); }
void RegisterLivePointers(const void* ptr, size_t size) {
  __lsan_register_root_region(ptr, size);
}
void UnRegisterLivePointers(const void* ptr, size_t size) {
  __lsan_unregister_root_region(ptr, size);
}
LeakCheckDisabler::LeakCheckDisabler() { __lsan_disable(); }
LeakCheckDisabler::~LeakCheckDisabler() { __lsan_enable(); }
TURBO_NAMESPACE_END
}  // namespace turbo

#else  // defined(TURBO_HAVE_LEAK_SANITIZER)

namespace turbo {
TURBO_NAMESPACE_BEGIN
bool HaveLeakSanitizer() { return false; }
bool LeakCheckerIsActive() { return false; }
void DoIgnoreLeak(const void*) { }
void RegisterLivePointers(const void*, size_t) { }
void UnRegisterLivePointers(const void*, size_t) { }
LeakCheckDisabler::LeakCheckDisabler() = default;
LeakCheckDisabler::~LeakCheckDisabler() = default;
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // defined(TURBO_HAVE_LEAK_SANITIZER)
