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
// Created by jeff on 24-6-4.
//

#include <turbo/memory/mem_alloc.h>
#include <new>

namespace turbo {
// These are out of line to have __cpp_aligned_new not affect ABI.
    TURBO_ATTRIBUTE_RETURNS_NONNULL TURBO_ATTRIBUTE_RETURNS_NOALIAS void *
    allocate_buffer(size_t Size, size_t Alignment) {
        return ::operator new(Size
#ifdef __cpp_aligned_new
                ,
                              std::align_val_t(Alignment)
#endif
        );
    }

    void deallocate_buffer(void *Ptr, size_t Size, size_t Alignment) {
        ::operator delete(Ptr
#ifdef __cpp_sized_deallocation
                ,
                          Size
#endif
#ifdef __cpp_aligned_new
                ,
                          std::align_val_t(Alignment)
#endif
        );
    }
}  // namespace turbo