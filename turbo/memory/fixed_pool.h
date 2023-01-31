// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_MEMORY_FIXED_POOL_H_
#define TURBO_MEMORY_FIXED_POOL_H_

#include "turbo/platform/config.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

enum class FixedPoolType : uint32_t {
  // Do not use object pool at all.
  //
  // This type is normally used for debugging purpose. (Object pooling makes it
  // hard to tracing object creation, by disabling it, debugging can be easier.)
  kDisabled,

  // Cache objects in a thread local cache.
  //
  // This type has the highest performance if your object allocation /
  // deallocation is done evenly in every thread.
  //
  // No lock / synchronization is required for this type of pool
  kThreadLocal,

  // Cache a small amount of objects locally, and use a shared pool for threads
  // in the same NUMA Node.
  //
  // If your objects is allocated in one thread, but freed in other threads in
  // the same scheduling group. This type of pool might work better.
  kMemoryNodeShared,

  // Cache a small amount of objects locally, and the rest are cached in a
  // global pool.
  //
  // This type of pool might work not-as-good as the above ones, but if your
  // workload has no evident allocation / deallocation pattern, this type might
  // suit most.
  kGlobal
};

// Note that this pool uses thread-local cache. That is, it does not perform
// well in scenarios such as producer-consumer (in this case, the producer
// thread keeps allocating objects while the consumer thread keeps de-allocating
// objects, and nothing could be reused by either thread.). Be aware of this.

// You need to customize these parameters before using this object pool.

template <class T>
struct FixedPoolTraits {
  // Type of backend pool to be used for this type. Check comments in `PoolType`
  // for their explanation.
  //
  // static constexpr PoolType kType = ...;  // Or `kPoolType`?

  // If your type cannot be created by `new T()`, you can provide a factory
  // here.
  //
  // Leave it undefined if you don't need it.
  //
  // static T* Create() { ... }

  // If you type cannot be destroyed by `delete ptr`, you can provide a
  // customized deleter here.
  //
  // Leave it undefined if you don't need it.
  //
  // void Destroy(T* ptr) { ... }

  // Below are several hooks.
  //
  // For those hooks you don't need, leave them as not defined.

  // Hook for `Get`. It's called after an object is retrieved from the pool.
  // This hook can be used for resetting objects to a "clean" state so that
  // users won't need to reset objects themselves.
  //
  // static void OnGet(T*) { ... }

  // Hook for `Put`. It's called before an object is put into the pool. It can
  // be handy if you want to release specific precious resources (handle to
  // temporary file, for example) before the object is hold by the pool.
  //
  // static void OnPut(T*) { ... }

  // For type-specific arguments, see header for the corresponding backend.

  // ... TODO

  static_assert(sizeof(T) == 0,
                "You need to specialize `turbo::FixedPoolTraits` to "
                "specify parameters before using `fixed_pool::Xxx`.");
};

TURBO_NAMESPACE_END
}  // namespace turbo
#endif  // TURBO_MEMORY_FIXED_POOL_H_
