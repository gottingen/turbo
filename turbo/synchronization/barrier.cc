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

#include <turbo/synchronization/barrier.h>

#include <turbo/base/internal/raw_logging.h>
#include <turbo/synchronization/mutex.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// Return whether int *arg is zero.
static bool IsZero(void *arg) {
  return 0 == *reinterpret_cast<int *>(arg);
}

bool Barrier::Block() {
  MutexLock l(&this->lock_);

  this->num_to_block_--;
  if (this->num_to_block_ < 0) {
    TURBO_RAW_LOG(
        FATAL,
        "Block() called too many times.  num_to_block_=%d out of total=%d",
        this->num_to_block_, this->num_to_exit_);
  }

  this->lock_.Await(Condition(IsZero, &this->num_to_block_));

  // Determine which thread can safely delete this Barrier object
  this->num_to_exit_--;
  TURBO_RAW_CHECK(this->num_to_exit_ >= 0, "barrier underflow");

  // If num_to_exit_ == 0 then all other threads in the barrier have
  // exited the Wait() and have released the Mutex so this thread is
  // free to delete the barrier.
  return this->num_to_exit_ == 0;
}

TURBO_NAMESPACE_END
}  // namespace turbo
