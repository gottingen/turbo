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
// barrier.h
// -----------------------------------------------------------------------------

#ifndef TURBO_SYNCHRONIZATION_BARRIER_H_
#define TURBO_SYNCHRONIZATION_BARRIER_H_

#include <turbo/base/thread_annotations.h>
#include <turbo/synchronization/mutex.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// Barrier
//
// This class creates a barrier which blocks threads until a prespecified
// threshold of threads (`num_threads`) utilizes the barrier. A thread utilizes
// the `Barrier` by calling `Block()` on the barrier, which will block that
// thread; no call to `Block()` will return until `num_threads` threads have
// called it.
//
// Exactly one call to `Block()` will return `true`, which is then responsible
// for destroying the barrier; because stack allocation will cause the barrier
// to be deleted when it is out of scope, barriers should not be stack
// allocated.
//
// Example:
//
//   // Main thread creates a `Barrier`:
//   barrier = new Barrier(num_threads);
//
//   // Each participating thread could then call:
//   if (barrier->Block()) delete barrier;  // Exactly one call to `Block()`
//                                          // returns `true`; that call
//                                          // deletes the barrier.
class Barrier {
 public:
  // `num_threads` is the number of threads that will participate in the barrier
  explicit Barrier(int num_threads)
      : num_to_block_(num_threads), num_to_exit_(num_threads) {}

  Barrier(const Barrier&) = delete;
  Barrier& operator=(const Barrier&) = delete;

  // Barrier::Block()
  //
  // Blocks the current thread, and returns only when the `num_threads`
  // threshold of threads utilizing this barrier has been reached. `Block()`
  // returns `true` for precisely one caller, which may then destroy the
  // barrier.
  //
  // Memory ordering: For any threads X and Y, any action taken by X
  // before X calls `Block()` will be visible to Y after Y returns from
  // `Block()`.
  bool Block();

 private:
  Mutex lock_;
  int num_to_block_ TURBO_GUARDED_BY(lock_);
  int num_to_exit_ TURBO_GUARDED_BY(lock_);
};

TURBO_NAMESPACE_END
}  // namespace turbo
#endif  // TURBO_SYNCHRONIZATION_BARRIER_H_
