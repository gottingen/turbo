
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_CONTAINER_FLAT_HASH_MAP_H_
#define FLARE_CONTAINER_FLAT_HASH_MAP_H_

#include "flare/container/internal/raw_hash_set.h"

namespace flare {


    // -----------------------------------------------------------------------------
    // flare::flat_hash_map
    // -----------------------------------------------------------------------------
    //
    // An `flare::flat_hash_map<K, V>` is an unordered associative container which
    // has been optimized for both speed and memory footprint in most common use
    // cases. Its interface is similar to that of `std::unordered_map<K, V>` with
    // the following notable differences:
    //
    // * Supports heterogeneous lookup, through `find()`, `operator[]()` and
    //   `insert()`, provided that the map is provided a compatible heterogeneous
    //   hashing function and equality operator.
    // * Invalidates any references and pointers to elements within the table after
    //   `rehash()`.
    // * Contains a `capacity()` member function indicating the number of element
    //   slots (open, deleted, and empty) within the hash map.
    // * Returns `void` from the `_erase(iterator)` overload.
    // -----------------------------------------------------------------------------
    template<class K, class V, class Hash, class Eq, class Alloc> // default values in map_fwd_decl.h
    class flat_hash_map : public flare::priv::raw_hash_map<
            flare::priv::flat_hash_map_policy<K, V>,
            Hash, Eq, Alloc> {
        using Base = typename flat_hash_map::raw_hash_map;

    public:
        flat_hash_map() {}

#ifdef __INTEL_COMPILER
        using Base::raw_hash_map;
#else
        using Base::Base;
#endif
        using Base::begin;
        using Base::cbegin;
        using Base::cend;
        using Base::end;
        using Base::capacity;
        using Base::empty;
        using Base::max_size;
        using Base::size;
        using Base::clear;
        using Base::erase;
        using Base::insert;
        using Base::insert_or_assign;
        using Base::emplace;
        using Base::emplace_hint;
        using Base::try_emplace;
        using Base::extract;
        using Base::merge;
        using Base::swap;
        using Base::rehash;
        using Base::reserve;
        using Base::at;
        using Base::contains;
        using Base::count;
        using Base::equal_range;
        using Base::find;
        using Base::operator[];
        using Base::bucket_count;
        using Base::load_factor;
        using Base::max_load_factor;
        using Base::get_allocator;
        using Base::hash_function;
        using Base::hash;
        using Base::key_eq;
    };

    template<class K, class V, class Hash, class Eq, class Alloc> // default values in map_fwd_decl.h
    class case_ignored_flat_hash_map : public flare::priv::raw_hash_map<
            flare::priv::flat_hash_map_policy<K, V>,
            Hash, Eq, Alloc> {
        using Base = typename case_ignored_flat_hash_map::raw_hash_map;

    public:
        case_ignored_flat_hash_map() {}

#ifdef __INTEL_COMPILER
        using Base::raw_hash_map;
#else
        using Base::Base;
#endif
        using Base::begin;
        using Base::cbegin;
        using Base::cend;
        using Base::end;
        using Base::capacity;
        using Base::empty;
        using Base::max_size;
        using Base::size;
        using Base::clear;
        using Base::erase;
        using Base::insert;
        using Base::insert_or_assign;
        using Base::emplace;
        using Base::emplace_hint;
        using Base::try_emplace;
        using Base::extract;
        using Base::merge;
        using Base::swap;
        using Base::rehash;
        using Base::reserve;
        using Base::at;
        using Base::contains;
        using Base::count;
        using Base::equal_range;
        using Base::find;
        using Base::operator[];
        using Base::bucket_count;
        using Base::load_factor;
        using Base::max_load_factor;
        using Base::get_allocator;
        using Base::hash_function;
        using Base::hash;
        using Base::key_eq;
    };

}  // namespace flare

#endif  // FLARE_CONTAINER_FLAT_HASH_MAP_H_
