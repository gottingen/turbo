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

#include <turbo/synchronization/blocking_counter.h>

#include <atomic>

#include <turbo/base/internal/raw_logging.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace {

// Return whether int *arg is true.
bool IsDone(void *arg) { return *reinterpret_cast<bool *>(arg); }

}  // namespace

BlockingCounter::BlockingCounter(int initial_count)
    : count_(initial_count),
      num_waiting_(0),
      done_{initial_count == 0 ? true : false} {
  TURBO_RAW_CHECK(initial_count >= 0, "BlockingCounter initial_count negative");
}

bool BlockingCounter::DecrementCount() {
  int count = count_.fetch_sub(1, std::memory_order_acq_rel) - 1;
  TURBO_RAW_CHECK(count >= 0,
                 "BlockingCounter::DecrementCount() called too many times");
  if (count == 0) {
    MutexLock l(&lock_);
    done_ = true;
    return true;
  }
  return false;
}

void BlockingCounter::Wait() {
  MutexLock l(&this->lock_);

  // only one thread may call Wait(). To support more than one thread,
  // implement a counter num_to_exit, like in the Barrier class.
  TURBO_RAW_CHECK(num_waiting_ == 0, "multiple threads called Wait()");
  num_waiting_++;

  this->lock_.Await(Condition(IsDone, &this->done_));

  // At this point, we know that all threads executing DecrementCount
  // will not touch this object again.
  // Therefore, the thread calling this method is free to delete the object
  // after we return from this method.
}

TURBO_NAMESPACE_END
}  // namespace turbo
