//
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
#pragma once


#include <turbo/base/attributes.h>
#include <turbo/base/internal/raw_logging.h>
#include <cstdlib>

namespace turbo {

    TURBO_ATTRIBUTE_RETURNS_NONNULL inline void *safe_malloc(size_t Sz) {
        void *Result = std::malloc(Sz);
        if (Result == nullptr) {
            // It is implementation-defined whether allocation occurs if the space
            // requested is zero (ISO/IEC 9899:2018 7.22.3). Retry, requesting
            // non-zero, if the space requested was zero.
            if (Sz == 0)
                return safe_malloc(1);
            TURBO_RAW_LOG(FATAL, "Allocation failed");
        }
        return Result;
    }

    TURBO_ATTRIBUTE_RETURNS_NONNULL inline void *safe_calloc(size_t Count,
                                                            size_t Sz) {
        void *Result = std::calloc(Count, Sz);
        if (Result == nullptr) {
            // It is implementation-defined whether allocation occurs if the space
            // requested is zero (ISO/IEC 9899:2018 7.22.3). Retry, requesting
            // non-zero, if the space requested was zero.
            if (Count == 0 || Sz == 0)
                return safe_malloc(1);
            TURBO_RAW_LOG(FATAL, "Allocation failed");
        }
        return Result;
    }

    TURBO_ATTRIBUTE_RETURNS_NONNULL inline void *safe_realloc(void *Ptr, size_t Sz) {
        void *Result = std::realloc(Ptr, Sz);
        if (Result == nullptr) {
            // It is implementation-defined whether allocation occurs if the space
            // requested is zero (ISO/IEC 9899:2018 7.22.3). Retry, requesting
            // non-zero, if the space requested was zero.
            if (Sz == 0)
                return safe_malloc(1);
            TURBO_RAW_LOG(FATAL, "Allocation failed");
        }
        return Result;
    }

    /// Allocate a buffer of memory with the given size and alignment.
    ///
    /// When the compiler supports aligned operator new, this will use it to to
    /// handle even over-aligned allocations.
    ///
    /// However, this doesn't make any attempt to leverage the fancier techniques
    /// like posix_memalign due to portability. It is mostly intended to allow
    /// compatibility with platforms that, after aligned allocation was added, use
    /// reduced default alignment.
    TURBO_ATTRIBUTE_RETURNS_NONNULL TURBO_ATTRIBUTE_RETURNS_NOALIAS void *
    allocate_buffer(size_t Size, size_t Alignment);

    /// Deallocate a buffer of memory with the given size and alignment.
    ///
    /// If supported, this will used the sized delete operator. Also if supported,
    /// this will pass the alignment to the delete operator.
    ///
    /// The pointer must have been allocated with the corresponding new operator,
    /// most likely using the above helper.
    void deallocate_buffer(void *Ptr, size_t Size, size_t Alignment);

} // namespace llvm