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
// -----------------------------------------------------------------------------
// vlog_config.h
// -----------------------------------------------------------------------------
//
// This header file defines `VLogSite`, a public primitive that represents
// a callsite for the `VLOG` family of macros and related libraries.
// It also declares and defines multiple internal utilities used to implement
// `VLOG`, such as `VLogSiteManager`.

#ifndef TURBO_LOG_INTERNAL_VLOG_CONFIG_H_
#define TURBO_LOG_INTERNAL_VLOG_CONFIG_H_

// IWYU pragma: private, include "turbo/log/log.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/optimization.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

class SyntheticBinary;
class VLogSite;

int RegisterAndInitialize(VLogSite* v);
void UpdateVLogSites();
constexpr int kUseFlag = (std::numeric_limits<int16_t>::min)();

// Represents a unique callsite for a `VLOG()` or `VLOG_IS_ON()` call.
//
// Libraries that provide `VLOG`-like functionality should use this to
// efficiently handle --vmodule.
//
// VLogSite objects must not be destroyed until the program exits. Doing so will
// probably yield nasty segfaults in VLogSiteManager::UpdateLogSites(). The
// recommendation is to make all such objects function-local statics.
class VLogSite final {
 public:
  // `f` must not be destroyed until the program exits.
  explicit constexpr VLogSite(const char* f)
      : file_(f), v_(kUninitialized), next_(nullptr) {}
  VLogSite(const VLogSite&) = delete;
  VLogSite& operator=(const VLogSite&) = delete;

  // Inlining the function yields a ~3x performance improvement at the cost of a
  // 1.5x code size increase at the call site.
  // Takes locks but does not allocate memory.
  TURBO_ATTRIBUTE_ALWAYS_INLINE
  bool IsEnabled(int level) {
    int stale_v = v_.load(std::memory_order_relaxed);
    if (TURBO_PREDICT_TRUE(level > stale_v)) {
      return false;
    }

    // We put everything other than the fast path, i.e. vlogging is initialized
    // but not on, behind an out-of-line function to reduce code size.
    // "level" is almost always a call-site constant, so we can save a bit
    // of code space by special-casing for a few common levels.
#if TURBO_HAVE_BUILTIN(__builtin_constant_p) || defined(__GNUC__)
    if (__builtin_constant_p(level)) {
      if (level == 0) return SlowIsEnabled0(stale_v);
      if (level == 1) return SlowIsEnabled1(stale_v);
      if (level == 2) return SlowIsEnabled2(stale_v);
      if (level == 3) return SlowIsEnabled3(stale_v);
      if (level == 4) return SlowIsEnabled4(stale_v);
      if (level == 5) return SlowIsEnabled5(stale_v);
    }
#endif
    return SlowIsEnabled(stale_v, level);
  }

 private:
  friend int log_internal::RegisterAndInitialize(VLogSite* v);
  friend void log_internal::UpdateVLogSites();
  friend class log_internal::SyntheticBinary;
  static constexpr int kUninitialized = (std::numeric_limits<int>::max)();

  // SlowIsEnabled performs slower checks to determine whether a log site is
  // enabled. Because it is expected to be called somewhat rarely
  // (comparatively), it is not inlined to save on code size.
  //
  // Prerequisites to calling SlowIsEnabled:
  //   1) stale_v is uninitialized OR
  //   2) stale_v is initialized and >= level (meaning we must log).
  // Takes locks but does not allocate memory.
  TURBO_ATTRIBUTE_NOINLINE
  bool SlowIsEnabled(int stale_v, int level);
  TURBO_ATTRIBUTE_NOINLINE bool SlowIsEnabled0(int stale_v);
  TURBO_ATTRIBUTE_NOINLINE bool SlowIsEnabled1(int stale_v);
  TURBO_ATTRIBUTE_NOINLINE bool SlowIsEnabled2(int stale_v);
  TURBO_ATTRIBUTE_NOINLINE bool SlowIsEnabled3(int stale_v);
  TURBO_ATTRIBUTE_NOINLINE bool SlowIsEnabled4(int stale_v);
  TURBO_ATTRIBUTE_NOINLINE bool SlowIsEnabled5(int stale_v);

  // This object is too size-sensitive to use turbo::string_view.
  const char* const file_;
  std::atomic<int> v_;
  std::atomic<VLogSite*> next_;
};
static_assert(std::is_trivially_destructible<VLogSite>::value,
              "VLogSite must be trivially destructible");

// Returns the current verbose log level of `file`.
// Does not allocate memory.
int VLogLevel(turbo::string_view file);

// Registers a site `v` to get updated as `vmodule` and `v` change.  Also
// initializes the site based on their current values, and returns that result.
// Does not allocate memory.
int RegisterAndInitialize(VLogSite* v);

// Allocates memory.
void UpdateVLogSites();

// Completely overwrites the saved value of `vmodule`.
// Allocates memory.
void UpdateVModule(turbo::string_view vmodule);

// Updates the global verbosity level to `v` and returns the prior value.
// Allocates memory.
int UpdateGlobalVLogLevel(int v);

// Atomically prepends `module_pattern=log_level` to the start of vmodule.
// Returns the prior value for `module_pattern` if there was an exact match and
// `global_v` otherwise.
// Allocates memory.
int PrependVModule(turbo::string_view module_pattern, int log_level);

// Registers `on_update` to be called whenever `v` or `vmodule` change.
// Allocates memory.
void OnVLogVerbosityUpdate(std::function<void()> cb);

// Does not allocate memory.
VLogSite* SetVModuleListHeadForTestOnly(VLogSite* v);

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_VLOG_CONFIG_H_
