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

#include <turbo/synchronization/internal/win32_waiter.h>

#ifdef TURBO_INTERNAL_HAVE_WIN32_WAITER

#include <windows.h>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/internal/thread_identity.h>
#include <turbo/base/optimization.h>
#include <turbo/synchronization/internal/kernel_timeout.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr char Win32Waiter::kName[];
#endif

class Win32Waiter::WinHelper {
 public:
  static SRWLOCK *GetLock(Win32Waiter *w) {
    return reinterpret_cast<SRWLOCK *>(&w->mu_storage_);
  }

  static CONDITION_VARIABLE *GetCond(Win32Waiter *w) {
    return reinterpret_cast<CONDITION_VARIABLE *>(&w->cv_storage_);
  }

  static_assert(sizeof(SRWLOCK) == sizeof(void *),
                "`mu_storage_` does not have the same size as SRWLOCK");
  static_assert(alignof(SRWLOCK) == alignof(void *),
                "`mu_storage_` does not have the same alignment as SRWLOCK");

  static_assert(sizeof(CONDITION_VARIABLE) == sizeof(void *),
                "`TURBO_CONDITION_VARIABLE_STORAGE` does not have the same size "
                "as `CONDITION_VARIABLE`");
  static_assert(
      alignof(CONDITION_VARIABLE) == alignof(void *),
      "`cv_storage_` does not have the same alignment as `CONDITION_VARIABLE`");

  // The SRWLOCK and CONDITION_VARIABLE types must be trivially constructible
  // and destructible because we never call their constructors or destructors.
  static_assert(std::is_trivially_constructible<SRWLOCK>::value,
                "The `SRWLOCK` type must be trivially constructible");
  static_assert(
      std::is_trivially_constructible<CONDITION_VARIABLE>::value,
      "The `CONDITION_VARIABLE` type must be trivially constructible");
  static_assert(std::is_trivially_destructible<SRWLOCK>::value,
                "The `SRWLOCK` type must be trivially destructible");
  static_assert(std::is_trivially_destructible<CONDITION_VARIABLE>::value,
                "The `CONDITION_VARIABLE` type must be trivially destructible");
};

class LockHolder {
 public:
  explicit LockHolder(SRWLOCK* mu) : mu_(mu) {
    AcquireSRWLockExclusive(mu_);
  }

  LockHolder(const LockHolder&) = delete;
  LockHolder& operator=(const LockHolder&) = delete;

  ~LockHolder() {
    ReleaseSRWLockExclusive(mu_);
  }

 private:
  SRWLOCK* mu_;
};

Win32Waiter::Win32Waiter() {
  auto *mu = ::new (static_cast<void *>(&mu_storage_)) SRWLOCK;
  auto *cv = ::new (static_cast<void *>(&cv_storage_)) CONDITION_VARIABLE;
  InitializeSRWLock(mu);
  InitializeConditionVariable(cv);
  waiter_count_ = 0;
  wakeup_count_ = 0;
}

bool Win32Waiter::Wait(KernelTimeout t) {
  SRWLOCK *mu = WinHelper::GetLock(this);
  CONDITION_VARIABLE *cv = WinHelper::GetCond(this);

  LockHolder h(mu);
  ++waiter_count_;

  // Loop until we find a wakeup to consume or timeout.
  // Note that, since the thread ticker is just reset, we don't need to check
  // whether the thread is idle on the very first pass of the loop.
  bool first_pass = true;
  while (wakeup_count_ == 0) {
    if (!first_pass) MaybeBecomeIdle();
    // No wakeups available, time to wait.
    if (!SleepConditionVariableSRW(cv, mu, t.InMillisecondsFromNow(), 0)) {
      // GetLastError() returns a Win32 DWORD, but we assign to
      // unsigned long to simplify the TURBO_RAW_LOG case below.  The uniform
      // initialization guarantees this is not a narrowing conversion.
      const unsigned long err{GetLastError()};  // NOLINT(runtime/int)
      if (err == ERROR_TIMEOUT) {
        --waiter_count_;
        return false;
      } else {
        TURBO_RAW_LOG(FATAL, "SleepConditionVariableSRW failed: %lu", err);
      }
    }
    first_pass = false;
  }
  // Consume a wakeup and we're done.
  --wakeup_count_;
  --waiter_count_;
  return true;
}

void Win32Waiter::Post() {
  LockHolder h(WinHelper::GetLock(this));
  ++wakeup_count_;
  InternalCondVarPoke();
}

void Win32Waiter::Poke() {
  LockHolder h(WinHelper::GetLock(this));
  InternalCondVarPoke();
}

void Win32Waiter::InternalCondVarPoke() {
  if (waiter_count_ != 0) {
    WakeConditionVariable(WinHelper::GetCond(this));
  }
}

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_WIN32_WAITER
