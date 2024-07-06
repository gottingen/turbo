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

#include <turbo/base/prefetch.h>

#include <memory>

#include <gtest/gtest.h>

namespace {

// Below tests exercise the functions only to guarantee they compile and execute
// correctly. We make no attempt at verifying any prefetch instructions being
// generated and executed: we assume the various implementation in terms of
// __builtin_prefetch() or x86 intrinsics to be correct and well tested.

TEST(PrefetchTest, PrefetchToLocalCache_StackA) {
  char buf[100] = {};
  turbo::prefetch_to_local_cache(buf);
  turbo::prefetch_to_local_cache_nta(buf);
  turbo::prefetch_to_local_cache_for_write(buf);
}

TEST(PrefetchTest, PrefetchToLocalCache_Heap) {
  auto memory = std::make_unique<char[]>(200 << 10);
  memset(memory.get(), 0, 200 << 10);
  turbo::prefetch_to_local_cache(memory.get());
  turbo::prefetch_to_local_cache_nta(memory.get());
  turbo::prefetch_to_local_cache_for_write(memory.get());
  turbo::prefetch_to_local_cache(memory.get() + (50 << 10));
  turbo::prefetch_to_local_cache_nta(memory.get() + (50 << 10));
  turbo::prefetch_to_local_cache_for_write(memory.get() + (50 << 10));
  turbo::prefetch_to_local_cache(memory.get() + (100 << 10));
  turbo::prefetch_to_local_cache_nta(memory.get() + (100 << 10));
  turbo::prefetch_to_local_cache_for_write(memory.get() + (100 << 10));
  turbo::prefetch_to_local_cache(memory.get() + (150 << 10));
  turbo::prefetch_to_local_cache_nta(memory.get() + (150 << 10));
  turbo::prefetch_to_local_cache_for_write(memory.get() + (150 << 10));
}

TEST(PrefetchTest, PrefetchToLocalCache_Nullptr) {
  turbo::prefetch_to_local_cache(nullptr);
  turbo::prefetch_to_local_cache_nta(nullptr);
  turbo::prefetch_to_local_cache_for_write(nullptr);
}

TEST(PrefetchTest, PrefetchToLocalCache_InvalidPtr) {
  turbo::prefetch_to_local_cache(reinterpret_cast<const void*>(0x785326532L));
  turbo::prefetch_to_local_cache_nta(reinterpret_cast<const void*>(0x785326532L));
  turbo::prefetch_to_local_cache_for_write(reinterpret_cast<const void*>(0x78532L));
}

}  // namespace
