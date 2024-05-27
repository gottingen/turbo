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
// blocking_counter.h
// -----------------------------------------------------------------------------

#ifndef TURBO_SYNCHRONIZATION_BLOCKING_COUNTER_H_
#define TURBO_SYNCHRONIZATION_BLOCKING_COUNTER_H_

#include <atomic>

#include <turbo/base/thread_annotations.h>
#include <turbo/synchronization/mutex.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// BlockingCounter
//
// This class allows a thread to block for a pre-specified number of actions.
// `BlockingCounter` maintains a single non-negative abstract integer "count"
// with an initial value `initial_count`. A thread can then call `Wait()` on
// this blocking counter to block until the specified number of events occur;
// worker threads then call 'DecrementCount()` on the counter upon completion of
// their work. Once the counter's internal "count" reaches zero, the blocked
// thread unblocks.
//
// A `BlockingCounter` requires the following:
//     - its `initial_count` is non-negative.
//     - the number of calls to `DecrementCount()` on it is at most
//       `initial_count`.
//     - `Wait()` is called at most once on it.
//
// Given the above requirements, a `BlockingCounter` provides the following
// guarantees:
//     - Once its internal "count" reaches zero, no legal action on the object
//       can further change the value of "count".
//     - When `Wait()` returns, it is legal to destroy the `BlockingCounter`.
//     - When `Wait()` returns, the number of calls to `DecrementCount()` on
//       this blocking counter exactly equals `initial_count`.
//
// Example:
//     BlockingCounter bcount(N);         // there are N items of work
//     ... Allow worker threads to start.
//     ... On completing each work item, workers do:
//     ... bcount.DecrementCount();      // an item of work has been completed
//
//     bcount.Wait();                    // wait for all work to be complete
//
class BlockingCounter {
 public:
  explicit BlockingCounter(int initial_count);

  BlockingCounter(const BlockingCounter&) = delete;
  BlockingCounter& operator=(const BlockingCounter&) = delete;

  // BlockingCounter::DecrementCount()
  //
  // Decrements the counter's "count" by one, and return "count == 0". This
  // function requires that "count != 0" when it is called.
  //
  // Memory ordering: For any threads X and Y, any action taken by X
  // before it calls `DecrementCount()` is visible to thread Y after
  // Y's call to `DecrementCount()`, provided Y's call returns `true`.
  bool DecrementCount();

  // BlockingCounter::Wait()
  //
  // Blocks until the counter reaches zero. This function may be called at most
  // once. On return, `DecrementCount()` will have been called "initial_count"
  // times and the blocking counter may be destroyed.
  //
  // Memory ordering: For any threads X and Y, any action taken by X
  // before X calls `DecrementCount()` is visible to Y after Y returns
  // from `Wait()`.
  void Wait();

 private:
  Mutex lock_;
  std::atomic<int> count_;
  int num_waiting_ TURBO_GUARDED_BY(lock_);
  bool done_ TURBO_GUARDED_BY(lock_);
};

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_SYNCHRONIZATION_BLOCKING_COUNTER_H_
