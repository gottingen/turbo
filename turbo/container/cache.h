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
// Created by jeff on 24-6-8.
//

#pragma once

#include <turbo/container/cache/cache_policy.h>
#include <turbo/container/cache/cache_internal.h>
#include <turbo/container/cache/fifo_cache_policy.h>
#include <turbo/container/cache/lru_cache_policy.h>
#include <turbo/container/cache/lfu_cache_policy.h>

namespace turbo{

    template <typename Key, typename Value>
    using Cache = fixed_sized_cache<Key, Value, NoCachePolicy>;

    template <typename Key, typename Value>
    using LRUCache = fixed_sized_cache<Key, Value, LRUCachePolicy>;

    template <typename Key, typename Value>
    using LFUCache = fixed_sized_cache<Key, Value, LFUCachePolicy>;

    template <typename Key, typename Value>
    using FIFOCache = fixed_sized_cache<Key, Value, FIFOCachePolicy>;

}  // namespace turbo
