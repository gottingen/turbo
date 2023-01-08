

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_CONTAINER_INTERNAL_MAP_FWD_DECL_H_
#define FLARE_CONTAINER_INTERNAL_MAP_FWD_DECL_H_


#include <memory>
#include <utility>
#include <string>
#include <string_view>
#include "flare/strings/ascii.h"
#include "flare/strings/compare.h"

namespace flare {

    template<class T>
    struct hash;

    template<class T>
    struct equal_to;

    template<class T>
    struct less;

    template<class T>
    struct case_ignored_less;


    template<class T> using Allocator = typename std::allocator<T>;
    template<class T1, class T2> using Pair = typename std::pair<T1, T2>;

    class null_mutex;

    namespace priv {

        // The hash of an object of type T is computed by using flare::hash.
        template<class T, class E = void>
        struct hash_eq {
            using hash = flare::hash<T>;
            using eq = flare::equal_to<T>;
        };

        template <typename T>
        struct case_ignored_hasher;

        template <typename T>
        struct case_ignored_equal;

        template<class T>
        using hash_default_hash = typename priv::hash_eq<T>::hash;

        template<class T>
        using hash_default_eq = typename priv::hash_eq<T>::eq;

        // type alias for std::allocator so we can forward declare without including other headers
        template<class T>
        using Allocator = typename flare::Allocator<T>;

        // type alias for std::pair so we can forward declare without including other headers
        template<class T1, class T2>
        using Pair = typename flare::Pair<T1, T2>;

    }  // namespace priv

    // ------------- forward declarations for hash containers ----------------------------------
    template<class T,
            class Hash  = flare::priv::hash_default_hash<T>,
            class Eq    = flare::priv::hash_default_eq<T>,
            class Alloc = flare::priv::Allocator<T>>  // alias for std::allocator
    class flat_hash_set;

    template<class T,
            class Hash  = flare::priv::case_ignored_hasher<T>,
            class Eq    = flare::priv::case_ignored_equal<T>,
            class Alloc = flare::priv::Allocator<T>>  // alias for std::allocator
    class case_ignored_flat_hash_set;

    template<class K, class V,
            class Hash  = flare::priv::hash_default_hash<K>,
            class Eq    = flare::priv::hash_default_eq<K>,
            class Alloc = flare::priv::Allocator<
                    flare::priv::Pair<const K, V>>> // alias for std::allocator
    class flat_hash_map;

    template<class K, class V,
            class Hash  = flare::priv::case_ignored_hasher<K>,
            class Eq    = flare::priv::case_ignored_equal<K>,
            class Alloc = flare::priv::Allocator<
                    flare::priv::Pair<const K, V>>> // alias for std::allocator
    class case_ignored_flat_hash_map;

    template<class T,
            class Hash  = flare::priv::hash_default_hash<T>,
            class Eq    = flare::priv::hash_default_eq<T>,
            class Alloc = flare::priv::Allocator<T>> // alias for std::allocator
    class node_hash_set;

    template<class T,
            class Hash  = flare::priv::case_ignored_hasher<T>,
            class Eq    = flare::priv::case_ignored_equal<T>,
            class Alloc = flare::priv::Allocator<T>> // alias for std::allocator
    class case_ignored_node_hash_set;

    template<class Key, class Value,
            class Hash  = flare::priv::hash_default_hash<Key>,
            class Eq    = flare::priv::hash_default_eq<Key>,
            class Alloc = flare::priv::Allocator<
                    flare::priv::Pair<const Key, Value>>> // alias for std::allocator
    class node_hash_map;

    template<class Key, class Value,
            class Hash  = flare::priv::case_ignored_hasher<Key>,
            class Eq    = flare::priv::case_ignored_equal<Key>,
            class Alloc = flare::priv::Allocator<
                    flare::priv::Pair<const Key, Value>>> // alias for std::allocator
    class case_ignored_node_hash_map;

    template<class T,
            class Hash  = flare::priv::hash_default_hash<T>,
            class Eq    = flare::priv::hash_default_eq<T>,
            class Alloc = flare::priv::Allocator<T>, // alias for std::allocator
            size_t N = 4,                  // 2**N submaps
            class Mutex = flare::null_mutex>   // use std::mutex to enable internal locks
    class parallel_flat_hash_set;

    template<class T,
            class Hash  = flare::priv::case_ignored_hasher<T>,
            class Eq    = flare::priv::case_ignored_equal<T>,
            class Alloc = flare::priv::Allocator<T>, // alias for std::allocator
            size_t N = 4,                  // 2**N submaps
            class Mutex = flare::null_mutex>   // use std::mutex to enable internal locks
    class case_ignored_parallel_flat_hash_set;

    template<class K, class V,
            class Hash  = flare::priv::hash_default_hash<K>,
            class Eq    = flare::priv::hash_default_eq<K>,
            class Alloc = flare::priv::Allocator<
                    flare::priv::Pair<const K, V>>, // alias for std::allocator
            size_t N = 4,                  // 2**N submaps
            class Mutex = flare::null_mutex>   // use std::mutex to enable internal locks
    class parallel_flat_hash_map;

    template<class K, class V,
            class Hash  = flare::priv::case_ignored_hasher<K>,
            class Eq    = flare::priv::case_ignored_equal<K>,
            class Alloc = flare::priv::Allocator<
                    flare::priv::Pair<const K, V>>, // alias for std::allocator
            size_t N = 4,                  // 2**N submaps
            class Mutex = flare::null_mutex>   // use std::mutex to enable internal locks
    class case_ignored_parallel_flat_hash_map;


    template<class T,
            class Hash  = flare::priv::hash_default_hash<T>,
            class Eq    = flare::priv::hash_default_eq<T>,
            class Alloc = flare::priv::Allocator<T>, // alias for std::allocator
            size_t N = 4,                  // 2**N submaps
            class Mutex = flare::null_mutex>   // use std::mutex to enable internal locks
    class parallel_node_hash_set;

    template<class T,
            class Hash  = flare::priv::case_ignored_hasher<T>,
            class Eq    = flare::priv::case_ignored_equal<T>,
            class Alloc = flare::priv::Allocator<T>, // alias for std::allocator
            size_t N = 4,                  // 2**N submaps
            class Mutex = flare::null_mutex>   // use std::mutex to enable internal locks
    class case_ignored_parallel_node_hash_set;

    template<class Key, class Value,
            class Hash  = flare::priv::hash_default_hash<Key>,
            class Eq    = flare::priv::hash_default_eq<Key>,
            class Alloc = flare::priv::Allocator<
                    flare::priv::Pair<const Key, Value>>, // alias for std::allocator
            size_t N = 4,                  // 2**N submaps
            class Mutex = flare::null_mutex>   // use std::mutex to enable internal locks
    class parallel_node_hash_map;

    template<class Key, class Value,
            class Hash  = flare::priv::case_ignored_hasher<Key>,
            class Eq    = flare::priv::case_ignored_equal<Key>,
            class Alloc = flare::priv::Allocator<
                    flare::priv::Pair<const Key, Value>>, // alias for std::allocator
            size_t N = 4,                  // 2**N submaps
            class Mutex = flare::null_mutex>   // use std::mutex to enable internal locks
    class case_ignored_parallel_node_hash_map;


    // ------------- forward declarations for btree containers ----------------------------------
    template<typename Key, typename Compare = flare::less<Key>,
            typename Alloc = flare::Allocator<Key>>
    class btree_set;

    template<typename Key, typename Compare = flare::case_ignored_less<Key>,
            typename Alloc = flare::Allocator<Key>>
    class case_ignored_btree_set;

    template<typename Key, typename Compare = flare::less<Key>,
            typename Alloc = flare::Allocator<Key>>
    class btree_multiset;

    template<typename Key, typename Compare = flare::case_ignored_less<Key>,
            typename Alloc = flare::Allocator<Key>>
    class case_ignored_btree_multiset;


    template<typename Key, typename Value, typename Compare = flare::less<Key>,
            typename Alloc = flare::Allocator<flare::priv::Pair<const Key, Value>>>
    class btree_map;

    template<typename Key, typename Value, typename Compare = flare::case_ignored_less<Key>,
            typename Alloc = flare::Allocator<flare::priv::Pair<const Key, Value>>>
    class case_ignored_btree_map;


    template<typename Key, typename Value, typename Compare = flare::less<Key>,
            typename Alloc = flare::Allocator<flare::priv::Pair<const Key, Value>>>
    class btree_multimap;

    template<typename Key, typename Value, typename Compare = flare::case_ignored_less<Key>,
            typename Alloc = flare::Allocator<flare::priv::Pair<const Key, Value>>>
    class case_ignored_btree_multimap;

}  // namespace flare



#endif  // FLARE_CONTAINER_INTERNAL_MAP_FWD_DECL_H_
