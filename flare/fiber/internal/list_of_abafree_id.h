// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: Mon Jun 20 11:57:23 CST 2016

#ifndef FLARE_FIBER_INTERNAL_LIST_OF_ABAFREE_ID_H_
#define FLARE_FIBER_INTERNAL_LIST_OF_ABAFREE_ID_H_

#include <vector>
#include <deque>

namespace flare::fiber_internal {

// A container for storing identifiers that may be invalidated.

// [Basic Idea]
// identifiers are remembered for error notifications. While insertions are 
// easy, removals are hard to be done in O(1) time. More importantly, 
// insertions are often done in one thread, while removals come from many
// threads simultaneously. Think about the usage in flare::rpc::Socket, most
// fiber_token_t are inserted by one thread (the thread calling non-contended
// Write or the KeepWrite thread), but removals are from many threads 
// processing responses simultaneously.

// [The approach]
// Don't remove old identifiers eagerly, replace them when new ones are inserted.

// token_traits MUST have {
//   // #identifiers in each block
//   static const size_t BLOCK_SIZE = 63;
//
//   // Max #entries. Often has close relationship with concurrency, 65536
//   // is "huge" for most apps.
//   static const size_t MAX_ENTRIES = 65536;
//
//   // Initial value of id. Id with the value is treated as invalid.
//   static const Id TOKEN_INIT = ...;
//
//   // Returns true if the id is valid. The "validness" must be permanent or
//   // stable for a very long period (to make the id ABA-free).
//   static bool exists(Id id);
// }

// This container is NOT thread-safe right now, and shouldn't be
// an issue in current usages throughout flare.
    template<typename Id, typename token_traits>
    class ListOfABAFreeId {
    public:
        ListOfABAFreeId();

        ~ListOfABAFreeId();

        // Add an identifier into the list.
        int add(Id id);

        // Apply fn(id) to all identifiers.
        template<typename Fn>
        void apply(const Fn &fn);

        // Put #entries of each level into `counts'
        // Returns #levels.
        size_t get_sizes(size_t *counts, size_t n);

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(ListOfABAFreeId);

        struct IdBlock {
            Id ids[token_traits::BLOCK_SIZE];
            IdBlock *next;
        };

        void forward_index();

        IdBlock *_cur_block;
        uint32_t _cur_index;
        uint32_t _nblock;
        IdBlock _head_block;
    };

// [impl.]

    template<typename Id, typename token_traits>
    ListOfABAFreeId<Id, token_traits>::ListOfABAFreeId()
            : _cur_block(&_head_block), _cur_index(0), _nblock(1) {
        for (size_t i = 0; i < token_traits::BLOCK_SIZE; ++i) {
            _head_block.ids[i] = token_traits::TOKEN_INIT;
        }
        _head_block.next = nullptr;
    }

    template<typename Id, typename token_traits>
    ListOfABAFreeId<Id, token_traits>::~ListOfABAFreeId() {
        _cur_block = nullptr;
        _cur_index = 0;
        _nblock = 0;
        for (IdBlock *p = _head_block.next; p != nullptr;) {
            IdBlock *saved_next = p->next;
            delete p;
            p = saved_next;
        }
        _head_block.next = nullptr;
    }

    template<typename Id, typename token_traits>
    inline void ListOfABAFreeId<Id, token_traits>::forward_index() {
        if (++_cur_index >= token_traits::BLOCK_SIZE) {
            _cur_index = 0;
            if (_cur_block->next) {
                _cur_block = _cur_block->next;
            } else {
                _cur_block = &_head_block;
            }
        }
    }

    template<typename Id, typename token_traits>
    int ListOfABAFreeId<Id, token_traits>::add(Id id) {
        // Scan for at most 4 positions, if any of them is empty, use the position.
        Id *saved_pos[4];
        for (size_t i = 0; i < FLARE_ARRAY_SIZE(saved_pos); ++i) {
            Id *const pos = _cur_block->ids + _cur_index;
            forward_index();
            // The position is not used.
            if (*pos == token_traits::TOKEN_INIT || !token_traits::exists(*pos)) {
                *pos = id;
                return 0;
            }
            saved_pos[i] = pos;
        }
        // The list is considered to be "crowded", add a new block and scatter
        // the conflict identifiers by inserting an empty entry after each of
        // them, so that even if the identifiers are still valid when we walk
        // through the area again, we can find an empty entry.

        // The new block is inserted as if it's inserted between xxxx and yyyy,
        // where xxxx are the 4 conflict identifiers.
        //  [..xxxxyyyy] -> [..........]
        //    block A        block B
        //
        //  [..xxxx....] -> [......yyyy] -> [..........]
        //    block A        new block      block B
        if (_nblock * token_traits::BLOCK_SIZE > token_traits::MAX_ENTRIES) {
            return EAGAIN;
        }
        IdBlock *new_block = new(std::nothrow) IdBlock;
        if (nullptr == new_block) {
            return ENOMEM;
        }
        ++_nblock;
        for (size_t i = 0; i < _cur_index; ++i) {
            new_block->ids[i] = token_traits::TOKEN_INIT;
        }
        for (size_t i = _cur_index; i < token_traits::BLOCK_SIZE; ++i) {
            new_block->ids[i] = _cur_block->ids[i];
            _cur_block->ids[i] = token_traits::TOKEN_INIT;
        }
        new_block->next = _cur_block->next;
        _cur_block->next = new_block;
        // Scatter the conflict identifiers.
        //  [..xxxx....] -> [......yyyy] -> [..........]
        //    block A        new block      block B
        //
        //  [..x.x.x.x.] -> [......yyyy] -> [..........]
        //    block A        new block      block B
        _cur_block->ids[_cur_index] = *saved_pos[2];
        *saved_pos[2] = *saved_pos[1];
        *saved_pos[1] = token_traits::TOKEN_INIT;
        forward_index();
        forward_index();
        _cur_block->ids[_cur_index] = *saved_pos[3];
        *saved_pos[3] = token_traits::TOKEN_INIT;
        forward_index();
        _cur_block->ids[_cur_index] = id;
        forward_index();
        return 0;
    }

    template<typename Id, typename token_traits>
    template<typename Fn>
    void ListOfABAFreeId<Id, token_traits>::apply(const Fn &fn) {
        for (IdBlock *p = &_head_block; p != nullptr; p = p->next) {
            for (size_t i = 0; i < token_traits::BLOCK_SIZE; ++i) {
                if (p->ids[i] != token_traits::TOKEN_INIT && token_traits::exists(p->ids[i])) {
                    fn(p->ids[i]);
                }
            }
        }
    }

    template<typename Id, typename token_traits>
    size_t ListOfABAFreeId<Id, token_traits>::get_sizes(size_t *cnts, size_t n) {
        if (n == 0) {
            return 0;
        }
        // Current impl. only has one level.
        cnts[0] = _nblock * token_traits::BLOCK_SIZE;
        return 1;
    }

}  // namespace flare::fiber_internal

#endif  // FLARE_FIBER_INTERNAL_LIST_OF_ABAFREE_ID_H_
