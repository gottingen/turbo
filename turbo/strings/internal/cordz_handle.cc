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
#include <turbo/strings/internal/cordz_handle.h>

#include <atomic>

#include <turbo/base/internal/raw_logging.h>  // For TURBO_RAW_CHECK
#include <turbo/base/no_destructor.h>
#include <turbo/synchronization/mutex.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

namespace {

struct Queue {
  Queue() = default;

  turbo::Mutex mutex;
  std::atomic<CordzHandle*> dq_tail TURBO_GUARDED_BY(mutex){nullptr};

  // Returns true if this delete queue is empty. This method does not acquire
  // the lock, but does a 'load acquire' observation on the delete queue tail.
  // It is used inside Delete() to check for the presence of a delete queue
  // without holding the lock. The assumption is that the caller is in the
  // state of 'being deleted', and can not be newly discovered by a concurrent
  // 'being constructed' snapshot instance. Practically, this means that any
  // such discovery (`find`, 'first' or 'next', etc) must have proper 'happens
  // before / after' semantics and atomic fences.
  bool IsEmpty() const TURBO_NO_THREAD_SAFETY_ANALYSIS {
    return dq_tail.load(std::memory_order_acquire) == nullptr;
  }
};

static Queue& GlobalQueue() {
  static turbo::NoDestructor<Queue> global_queue;
  return *global_queue;
}

}  // namespace

CordzHandle::CordzHandle(bool is_snapshot) : is_snapshot_(is_snapshot) {
  Queue& global_queue = GlobalQueue();
  if (is_snapshot) {
    MutexLock lock(&global_queue.mutex);
    CordzHandle* dq_tail = global_queue.dq_tail.load(std::memory_order_acquire);
    if (dq_tail != nullptr) {
      dq_prev_ = dq_tail;
      dq_tail->dq_next_ = this;
    }
    global_queue.dq_tail.store(this, std::memory_order_release);
  }
}

CordzHandle::~CordzHandle() {
  Queue& global_queue = GlobalQueue();
  if (is_snapshot_) {
    std::vector<CordzHandle*> to_delete;
    {
      MutexLock lock(&global_queue.mutex);
      CordzHandle* next = dq_next_;
      if (dq_prev_ == nullptr) {
        // We were head of the queue, delete every CordzHandle until we reach
        // either the end of the list, or a snapshot handle.
        while (next && !next->is_snapshot_) {
          to_delete.push_back(next);
          next = next->dq_next_;
        }
      } else {
        // Another CordzHandle existed before this one, don't delete anything.
        dq_prev_->dq_next_ = next;
      }
      if (next) {
        next->dq_prev_ = dq_prev_;
      } else {
        global_queue.dq_tail.store(dq_prev_, std::memory_order_release);
      }
    }
    for (CordzHandle* handle : to_delete) {
      delete handle;
    }
  }
}

bool CordzHandle::SafeToDelete() const {
  return is_snapshot_ || GlobalQueue().IsEmpty();
}

void CordzHandle::Delete(CordzHandle* handle) {
  assert(handle);
  if (handle) {
    Queue& queue = GlobalQueue();
    if (!handle->SafeToDelete()) {
      MutexLock lock(&queue.mutex);
      CordzHandle* dq_tail = queue.dq_tail.load(std::memory_order_acquire);
      if (dq_tail != nullptr) {
        handle->dq_prev_ = dq_tail;
        dq_tail->dq_next_ = handle;
        queue.dq_tail.store(handle, std::memory_order_release);
        return;
      }
    }
    delete handle;
  }
}

std::vector<const CordzHandle*> CordzHandle::DiagnosticsGetDeleteQueue() {
  std::vector<const CordzHandle*> handles;
  Queue& global_queue = GlobalQueue();
  MutexLock lock(&global_queue.mutex);
  CordzHandle* dq_tail = global_queue.dq_tail.load(std::memory_order_acquire);
  for (const CordzHandle* p = dq_tail; p; p = p->dq_prev_) {
    handles.push_back(p);
  }
  return handles;
}

bool CordzHandle::DiagnosticsHandleIsSafeToInspect(
    const CordzHandle* handle) const {
  if (!is_snapshot_) return false;
  if (handle == nullptr) return true;
  if (handle->is_snapshot_) return false;
  bool snapshot_found = false;
  Queue& global_queue = GlobalQueue();
  MutexLock lock(&global_queue.mutex);
  for (const CordzHandle* p = global_queue.dq_tail; p; p = p->dq_prev_) {
    if (p == handle) return !snapshot_found;
    if (p == this) snapshot_found = true;
  }
  TURBO_ASSERT(snapshot_found);  // Assert that 'this' is in delete queue.
  return true;
}

std::vector<const CordzHandle*>
CordzHandle::DiagnosticsGetSafeToInspectDeletedHandles() {
  std::vector<const CordzHandle*> handles;
  if (!is_snapshot()) {
    return handles;
  }

  Queue& global_queue = GlobalQueue();
  MutexLock lock(&global_queue.mutex);
  for (const CordzHandle* p = dq_next_; p != nullptr; p = p->dq_next_) {
    if (!p->is_snapshot()) {
      handles.push_back(p);
    }
  }
  return handles;
}

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
