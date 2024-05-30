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

// Interface for getting the current ThreadIdentity, creating one if necessary.
// See thread_identity.h.
//
// This file is separate from thread_identity.h because creating a new
// ThreadIdentity requires slightly higher level libraries (per_thread_sem
// and low_level_alloc) than accessing an existing one.  This separation allows
// us to have a smaller //turbo/base:base.

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_CREATE_THREAD_IDENTITY_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_CREATE_THREAD_IDENTITY_H_

#include <turbo/base/internal/thread_identity.h>
#include <turbo/base/port.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

// Allocates and attaches a ThreadIdentity object for the calling thread.
// For private use only.
base_internal::ThreadIdentity* CreateThreadIdentity();

// Returns the ThreadIdentity object representing the calling thread; guaranteed
// to be unique for its lifetime.  The returned object will remain valid for the
// program's lifetime; although it may be re-assigned to a subsequent thread.
// If one does not exist for the calling thread, allocate it now.
inline base_internal::ThreadIdentity* GetOrCreateCurrentThreadIdentity() {
  base_internal::ThreadIdentity* identity =
      base_internal::CurrentThreadIdentityIfPresent();
  if (TURBO_UNLIKELY(identity == nullptr)) {
    return CreateThreadIdentity();
  }
  return identity;
}

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_CREATE_THREAD_IDENTITY_H_
