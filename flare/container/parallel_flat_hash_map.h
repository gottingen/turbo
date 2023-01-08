
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_CONTAINER_PARALLEL_FLAT_HASH_MAP_H_
#define FLARE_CONTAINER_PARALLEL_FLAT_HASH_MAP_H_

#include "flare/container/internal/raw_hash_set.h"

namespace flare {

    // -----------------------------------------------------------------------------
    // flare::parallel_flat_hash_map - default values in map_fwd_decl.h
    // -----------------------------------------------------------------------------
    template<class K, class V, class Hash, class Eq, class Alloc, size_t N, class Mtx_>
    class parallel_flat_hash_map : public flare::priv::parallel_hash_map<
            N, flare::priv::raw_hash_set, Mtx_,
            flare::priv::flat_hash_map_policy<K, V>,
            Hash, Eq, Alloc> {
        using Base = typename parallel_flat_hash_map::parallel_hash_map;

    public:
        parallel_flat_hash_map() {}

#ifdef __INTEL_COMPILER
        using Base::parallel_hash_map;
#else
        using Base::Base;
#endif
        using Base::hash;
        using Base::subidx;
        using Base::subcnt;
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
        using Base::emplace_with_hash;
        using Base::emplace_hint_with_hash;
        using Base::try_emplace_with_hash;
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
        using Base::key_eq;
    };

    template<class K, class V, class Hash, class Eq, class Alloc, size_t N, class Mtx_>
    class case_ignored_parallel_flat_hash_map : public flare::priv::parallel_hash_map<
            N, flare::priv::raw_hash_set, Mtx_,
            flare::priv::flat_hash_map_policy<K, V>,
            Hash, Eq, Alloc> {
        using Base = typename case_ignored_parallel_flat_hash_map::parallel_hash_map;

    public:
        case_ignored_parallel_flat_hash_map() {}

#ifdef __INTEL_COMPILER
        using Base::parallel_hash_map;
#else
        using Base::Base;
#endif
        using Base::hash;
        using Base::subidx;
        using Base::subcnt;
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
        using Base::emplace_with_hash;
        using Base::emplace_hint_with_hash;
        using Base::try_emplace_with_hash;
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
        using Base::key_eq;
    };

}
#endif  // FLARE_CONTAINER_PARALLEL_FLAT_HASH_MAP_H_
