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

#include <stdlib.h>
#include <algorithm>
#include <turbo/memory/arena.h>

namespace turbo {

    ArenaOptions::ArenaOptions()
            : initial_block_size(64), max_block_size(8192) {}

    Arena::Arena(const ArenaOptions &options)
            : _cur_block(NULL), _isolated_blocks(NULL), _block_size(options.initial_block_size), _options(options) {
    }

    Arena::~Arena() {
        while (_cur_block != NULL) {
            Block *const saved_next = _cur_block->next;
            free(_cur_block);
            _cur_block = saved_next;
        }
        while (_isolated_blocks != NULL) {
            Block *const saved_next = _isolated_blocks->next;
            free(_isolated_blocks);
            _isolated_blocks = saved_next;
        }
    }

    void Arena::swap(Arena &other) {
        std::swap(_cur_block, other._cur_block);
        std::swap(_isolated_blocks, other._isolated_blocks);
        std::swap(_block_size, other._block_size);
        const ArenaOptions tmp = _options;
        _options = other._options;
        other._options = tmp;
    }

    void Arena::clear() {
        Arena a;
        swap(a);
    }

    void *Arena::allocate_new_block(size_t n) {
        Block *b = (Block *) malloc(offsetof(Block, data) + n);
        b->next = _isolated_blocks;
        b->alloc_size = n;
        b->size = n;
        _isolated_blocks = b;
        return b->data;
    }

    void *Arena::allocate_in_other_blocks(size_t n) {
        if (n > _block_size / 4) { // put outlier on separate blocks.
            return allocate_new_block(n);
        }
        // Waste the left space. At most 1/4 of allocated spaces are wasted.

        // Grow the block size gradually.
        if (_cur_block != NULL) {
            _block_size = std::min(2 * _block_size, _options.max_block_size);
        }
        size_t new_size = _block_size;
        if (new_size < n) {
            new_size = n;
        }
        Block *b = (Block *) malloc(offsetof(Block, data) + new_size);
        if (NULL == b) {
            return NULL;
        }
        b->next = NULL;
        b->alloc_size = n;
        b->size = new_size;
        if (_cur_block) {
            _cur_block->next = _isolated_blocks;
            _isolated_blocks = _cur_block;
        }
        _cur_block = b;
        return b->data;
    }

}  // namespace turbo
