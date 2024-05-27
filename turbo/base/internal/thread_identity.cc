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

#include <turbo/base/internal/thread_identity.h>

#if !defined(_WIN32) || defined(__MINGW32__)
#include <pthread.h>
#ifndef __wasi__
// WASI does not provide this header, either way we disable use
// of signals with it below.
#include <signal.h>
#endif
#endif

#include <atomic>
#include <cassert>
#include <memory>

#include <turbo/base/attributes.h>
#include <turbo/base/call_once.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/internal/spinlock.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

#if TURBO_THREAD_IDENTITY_MODE != TURBO_THREAD_IDENTITY_MODE_USE_CPP11
namespace {
// Used to co-ordinate one-time creation of our pthread_key
turbo::once_flag init_thread_identity_key_once;
pthread_key_t thread_identity_pthread_key;
std::atomic<bool> pthread_key_initialized(false);

void AllocateThreadIdentityKey(ThreadIdentityReclaimerFunction reclaimer) {
  pthread_key_create(&thread_identity_pthread_key, reclaimer);
  pthread_key_initialized.store(true, std::memory_order_release);
}
}  // namespace
#endif

#if TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_TLS || \
    TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_CPP11
// The actual TLS storage for a thread's currently associated ThreadIdentity.
// This is referenced by inline accessors in the header.
// "protected" visibility ensures that if multiple instances of Turbo code
// exist within a process (via dlopen() or similar), references to
// thread_identity_ptr from each instance of the code will refer to
// *different* instances of this ptr.
// Apple platforms have the visibility attribute, but issue a compile warning
// that protected visibility is unsupported.
TURBO_CONST_INIT  // Must come before __attribute__((visibility("protected")))
#if TURBO_HAVE_ATTRIBUTE(visibility) && !defined(__APPLE__)
    __attribute__((visibility("protected")))
#endif  // TURBO_HAVE_ATTRIBUTE(visibility) && !defined(__APPLE__)
#if TURBO_PER_THREAD_TLS
    // Prefer __thread to thread_local as benchmarks indicate it is a bit
    // faster.
    TURBO_PER_THREAD_TLS_KEYWORD ThreadIdentity* thread_identity_ptr = nullptr;
#elif defined(TURBO_HAVE_THREAD_LOCAL)
    thread_local ThreadIdentity* thread_identity_ptr = nullptr;
#endif  // TURBO_PER_THREAD_TLS
#endif  // TLS or CPP11

void SetCurrentThreadIdentity(ThreadIdentity* identity,
                              ThreadIdentityReclaimerFunction reclaimer) {
  assert(CurrentThreadIdentityIfPresent() == nullptr);
  // Associate our destructor.
  // NOTE: This call to pthread_setspecific is currently the only immovable
  // barrier to CurrentThreadIdentity() always being async signal safe.
#if TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
  // NOTE: Not async-safe.  But can be open-coded.
  turbo::call_once(init_thread_identity_key_once, AllocateThreadIdentityKey,
                  reclaimer);

#if defined(__wasi__) || defined(__EMSCRIPTEN__) || defined(__MINGW32__) || \
    defined(__hexagon__)
  // Emscripten, WASI and MinGW pthread implementations does not support
  // signals. See
  // https://kripken.github.io/emscripten-site/docs/porting/pthreads.html for
  // more information.
  pthread_setspecific(thread_identity_pthread_key,
                      reinterpret_cast<void*>(identity));
#else
  // We must mask signals around the call to setspecific as with current glibc,
  // a concurrent getspecific (needed for GetCurrentThreadIdentityIfPresent())
  // may zero our value.
  //
  // While not officially async-signal safe, getspecific within a signal handler
  // is otherwise OK.
  sigset_t all_signals;
  sigset_t curr_signals;
  sigfillset(&all_signals);
  pthread_sigmask(SIG_SETMASK, &all_signals, &curr_signals);
  pthread_setspecific(thread_identity_pthread_key,
                      reinterpret_cast<void*>(identity));
  pthread_sigmask(SIG_SETMASK, &curr_signals, nullptr);
#endif  // !__EMSCRIPTEN__ && !__MINGW32__

#elif TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_TLS
  // NOTE: Not async-safe.  But can be open-coded.
  turbo::call_once(init_thread_identity_key_once, AllocateThreadIdentityKey,
                  reclaimer);
  pthread_setspecific(thread_identity_pthread_key,
                      reinterpret_cast<void*>(identity));
  thread_identity_ptr = identity;
#elif TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_CPP11
  thread_local std::unique_ptr<ThreadIdentity, ThreadIdentityReclaimerFunction>
      holder(identity, reclaimer);
  thread_identity_ptr = identity;
#else
#error Unimplemented TURBO_THREAD_IDENTITY_MODE
#endif
}

#if TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_TLS || \
    TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_CPP11

// Please see the comment on `CurrentThreadIdentityIfPresent` in
// thread_identity.h. When we cannot expose thread_local variables in
// headers, we opt for the correct-but-slower option of not inlining this
// function.
#ifndef TURBO_INTERNAL_INLINE_CURRENT_THREAD_IDENTITY_IF_PRESENT
ThreadIdentity* CurrentThreadIdentityIfPresent() { return thread_identity_ptr; }
#endif
#endif

void ClearCurrentThreadIdentity() {
#if TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_TLS || \
    TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_CPP11
  thread_identity_ptr = nullptr;
#elif TURBO_THREAD_IDENTITY_MODE == \
    TURBO_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
  // pthread_setspecific expected to clear value on destruction
  assert(CurrentThreadIdentityIfPresent() == nullptr);
#endif
}

#if TURBO_THREAD_IDENTITY_MODE == TURBO_THREAD_IDENTITY_MODE_USE_POSIX_SETSPECIFIC
ThreadIdentity* CurrentThreadIdentityIfPresent() {
  bool initialized = pthread_key_initialized.load(std::memory_order_acquire);
  if (!initialized) {
    return nullptr;
  }
  return reinterpret_cast<ThreadIdentity*>(
      pthread_getspecific(thread_identity_pthread_key));
}
#endif

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo
