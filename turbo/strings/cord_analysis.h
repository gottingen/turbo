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

#pragma once

#include <cstddef>
#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/nullability.h>
#include <turbo/strings/internal/cord_internal.h>

namespace turbo {
    namespace cord_internal {

        // Returns the *approximate* number of bytes held in full or in part by this
        // Cord (which may not remain the same between invocations). Cords that share
        // memory could each be "charged" independently for the same shared memory.
        size_t GetEstimatedMemoryUsage(turbo::Nonnull<const CordRep *> rep);

        // Returns the *approximate* number of bytes held in full or in part by this
        // Cord for the distinct memory held by this cord. This is similar to
        // `GetEstimatedMemoryUsage()`, except that if the cord has multiple references
        // to the same memory, that memory is only counted once.
        //
        // For example:
        //   turbo::Cord cord;
        //   cord.append(some_other_cord);
        //   cord.append(some_other_cord);
        //    // Calls GetEstimatedMemoryUsage() and counts `other_cord` twice:
        //   cord.EstimatedMemoryUsage(kTotal);
        //    // Calls GetMorePreciseMemoryUsage() and counts `other_cord` once:
        //   cord.EstimatedMemoryUsage(kTotalMorePrecise);
        //
        // This is more expensive than `GetEstimatedMemoryUsage()` as it requires
        // deduplicating all memory references.
        size_t GetMorePreciseMemoryUsage(turbo::Nonnull<const CordRep *> rep);

        // Returns the *approximate* number of bytes held in full or in part by this
        // CordRep weighted by the sharing ratio of that data. For example, if some data
        // edge is shared by 4 different Cords, then each cord is attribute 1/4th of
        // the total memory usage as a 'fair share' of the total memory usage.
        size_t GetEstimatedFairShareMemoryUsage(turbo::Nonnull<const CordRep *> rep);

    }  // namespace cord_internal
}  // namespace turbo

