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

// Date: Sat Dec  3 13:11:32 CST 2016

#ifndef TEST_CONTAINER_POOLED_MAP_H_
#define TEST_CONTAINER_POOLED_MAP_H_

#include "flare/container/single_threaded_pool.h"
#include <new>
#include <map>

namespace flare::container {
    namespace details {
        template<class T1, size_t BLOCK_SIZE>
        class PooledAllocator;
    }

    template<typename K, typename V, size_t BLOCK_SIZE = 512,
            typename C = std::less<K> >
    class PooledMap
            : public std::map<K, V, C, details::PooledAllocator<std::pair<const K, V>, BLOCK_SIZE> > {

    };

    namespace details {
// Specialize for void
        template<size_t BLOCK_SIZE>
        class PooledAllocator<void, BLOCK_SIZE> {
        public:
            typedef void *pointer;
            typedef const void *const_pointer;
            typedef void value_type;
            template<class U1>
            struct rebind {
                typedef PooledAllocator<U1, BLOCK_SIZE> other;
            };
        };

        template<class T1, size_t BLOCK_SIZE>
        class PooledAllocator {
        public:
            typedef T1 value_type;
            typedef size_t size_type;
            typedef ptrdiff_t difference_type;
            typedef T1 *pointer;
            typedef const T1 *const_pointer;
            typedef T1 &reference;
            typedef const T1 &const_reference;
            template<class U1>
            struct rebind {
                typedef PooledAllocator<U1, BLOCK_SIZE> other;
            };

        public:
            PooledAllocator() {}

            PooledAllocator(const PooledAllocator &) {}

            template<typename U1, size_t BS2>
            PooledAllocator(const PooledAllocator<U1, BS2> &) {}

            void operator=(const PooledAllocator &) {}

            template<typename U1, size_t BS2>
            void operator=(const PooledAllocator<U1, BS2> &) {}

            void swap(PooledAllocator &other) { _pool.swap(other._pool); }

            // Convert references to pointers.
            pointer address(reference r) const { return &r; }

            const_pointer address(const_reference r) const { return &r; }

            // Allocate storage for n values of T1.
            pointer allocate(size_type n, PooledAllocator<void, 0>::const_pointer = 0) {
                if (n == 1) {
                    return (pointer) _pool.get();
                } else {
                    return (pointer) malloc(n * sizeof(T1));
                }
            }

            // Deallocate storage obtained by a call to allocate.
            void deallocate(pointer p, size_type n) {
                if (n == 1) {
                    return _pool.back(p);
                } else {
                    free(p);
                }
            }

            // Return the largest possible storage available through a call to allocate.
            size_type max_size() const { return 0xFFFFFFFF / sizeof(T1); }

            void construct(pointer ptr) { ::new(ptr) T1; }

            void construct(pointer ptr, const T1 &val) { ::new(ptr) T1(val); }

            template<class U1>
            void construct(pointer ptr, const U1 &val) { ::new(ptr) T1(val); }

            void destroy(pointer p) { p->T1::~T1(); }

        private:
            flare::container::SingleThreadedPool<sizeof(T1), BLOCK_SIZE, 1> _pool;
        };

        // Return true if b could be used to deallocate storage obtained through a
        // and vice versa. It's clear that our allocator can't be exchanged.
        template<typename T1, size_t S1, typename T2, size_t S2>
        bool operator==(const PooledAllocator<T1, S1> &, const PooledAllocator<T2, S2> &) { return false; }

        template<typename T1, size_t S1, typename T2, size_t S2>
        bool operator!=(const PooledAllocator<T1, S1> &a, const PooledAllocator<T2, S2> &b) { return !(a == b); }

    } // namespace details
} // namespace flare::container


#include <utility>    // std::swap since C++11

namespace std {
    template<class T1, size_t BLOCK_SIZE>
    inline void swap(::flare::container::details::PooledAllocator<T1, BLOCK_SIZE> &lhs,
                     ::flare::container::details::PooledAllocator<T1, BLOCK_SIZE> &rhs) {
        lhs.swap(rhs);
    }
}  // namespace std

#endif  // TEST_CONTAINER_POOLED_MAP_H_
