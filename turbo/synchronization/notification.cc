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

#include <turbo/synchronization/notification.h>

#include <atomic>

#include <turbo/base/internal/raw_logging.h>
#include <turbo/synchronization/mutex.h>
#include <turbo/time/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

void Notification::Notify() {
  MutexLock l(&this->mutex_);

#ifndef NDEBUG
  if (TURBO_PREDICT_FALSE(notified_yet_.load(std::memory_order_relaxed))) {
    TURBO_RAW_LOG(
        FATAL,
        "Notify() method called more than once for Notification object %p",
        static_cast<void *>(this));
  }
#endif

  notified_yet_.store(true, std::memory_order_release);
}

Notification::~Notification() {
  // Make sure that the thread running Notify() exits before the object is
  // destructed.
  MutexLock l(&this->mutex_);
}

void Notification::WaitForNotification() const {
  if (!HasBeenNotifiedInternal(&this->notified_yet_)) {
    this->mutex_.LockWhen(Condition(&HasBeenNotifiedInternal,
                                    &this->notified_yet_));
    this->mutex_.Unlock();
  }
}

bool Notification::WaitForNotificationWithTimeout(
    turbo::Duration timeout) const {
  bool notified = HasBeenNotifiedInternal(&this->notified_yet_);
  if (!notified) {
    notified = this->mutex_.LockWhenWithTimeout(
        Condition(&HasBeenNotifiedInternal, &this->notified_yet_), timeout);
    this->mutex_.Unlock();
  }
  return notified;
}

bool Notification::WaitForNotificationWithDeadline(turbo::Time deadline) const {
  bool notified = HasBeenNotifiedInternal(&this->notified_yet_);
  if (!notified) {
    notified = this->mutex_.LockWhenWithDeadline(
        Condition(&HasBeenNotifiedInternal, &this->notified_yet_), deadline);
    this->mutex_.Unlock();
  }
  return notified;
}

TURBO_NAMESPACE_END
}  // namespace turbo
