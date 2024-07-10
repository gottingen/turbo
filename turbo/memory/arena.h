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


// Do small memory allocations on continuous blocks.

#pragma once

#include <cstdint>
#include <cstddef>

namespace turbo {

    struct ArenaOptions {
        size_t initial_block_size;
        size_t max_block_size;

        // Constructed with default options.
        ArenaOptions();
    };

    // Just a proof-of-concept, will be refactored in future CI.
    class Arena {
    public:
        explicit Arena(const ArenaOptions &options = ArenaOptions());

        ~Arena();

        void swap(Arena &);

        void *allocate(size_t n);

        void *allocate_aligned(size_t n);  // not implemented.
        void clear();

        Arena(const Arena &) = delete;

        Arena &operator=(const Arena &) = delete;

    private:

        struct Block {
            uint32_t left_space() const { return size - alloc_size; }

            Block *next;
            uint32_t alloc_size;
            uint32_t size;
            char data[0];
        };

        void *allocate_in_other_blocks(size_t n);

        void *allocate_new_block(size_t n);

        Block *pop_block(Block *&head) {
            Block *saved_head = head;
            head = head->next;
            return saved_head;
        }

        Block *_cur_block;
        Block *_isolated_blocks;
        size_t _block_size;
        ArenaOptions _options;
    };

    inline void *Arena::allocate(size_t n) {
        if (_cur_block != NULL && _cur_block->left_space() >= n) {
            void *ret = _cur_block->data + _cur_block->alloc_size;
            _cur_block->alloc_size += n;
            return ret;
        }
        return allocate_in_other_blocks(n);
    }

}  // namespace turbo
