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

#include <turbo/base/internal/low_level_alloc.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>  // NOLINT(build/c++11)
#include <unordered_map>
#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <turbo/container/node_hash_map.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {
namespace {

// This test doesn't use gtest since it needs to test that everything
// works before main().
#define TEST_ASSERT(x)                                           \
  if (!(x)) {                                                    \
    printf("TEST_ASSERT(%s) FAILED ON LINE %d\n", #x, __LINE__); \
    abort();                                                     \
  }

// a block of memory obtained from the allocator
struct BlockDesc {
  char *ptr;      // pointer to memory
  int len;        // number of bytes
  int fill;       // filled with data starting with this
};

// Check that the pattern placed in the block d
// by RandomizeBlockDesc is still there.
static void CheckBlockDesc(const BlockDesc &d) {
  for (int i = 0; i != d.len; i++) {
    TEST_ASSERT((d.ptr[i] & 0xff) == ((d.fill + i) & 0xff));
  }
}

// Fill the block "*d" with a pattern
// starting with a random byte.
static void RandomizeBlockDesc(BlockDesc *d) {
  d->fill = rand() & 0xff;
  for (int i = 0; i != d->len; i++) {
    d->ptr[i] = (d->fill + i) & 0xff;
  }
}

// Use to indicate to the malloc hooks that
// this calls is from LowLevelAlloc.
static bool using_low_level_alloc = false;

// n times, toss a coin, and based on the outcome
// either allocate a new block or deallocate an old block.
// New blocks are placed in a std::unordered_map with a random key
// and initialized with RandomizeBlockDesc().
// If keys conflict, the older block is freed.
// Old blocks are always checked with CheckBlockDesc()
// before being freed.  At the end of the run,
// all remaining allocated blocks are freed.
// If use_new_arena is true, use a fresh arena, and then delete it.
// If call_malloc_hook is true and user_arena is true,
// allocations and deallocations are reported via the MallocHook
// interface.
static void Test(bool use_new_arena, bool call_malloc_hook, int n) {
  typedef turbo::node_hash_map<int, BlockDesc> AllocMap;
  AllocMap allocated;
  AllocMap::iterator it;
  BlockDesc block_desc;
  int rnd;
  LowLevelAlloc::Arena *arena = nullptr;
  if (use_new_arena) {
    int32_t flags = call_malloc_hook ? LowLevelAlloc::kCallMallocHook : 0;
    arena = LowLevelAlloc::NewArena(flags);
  }
  for (int i = 0; i != n; i++) {
    if (i != 0 && i % 10000 == 0) {
      printf(".");
      fflush(stdout);
    }

    switch (rand() & 1) {      // toss a coin
    case 0:     // coin came up heads: add a block
      using_low_level_alloc = true;
      block_desc.len = rand() & 0x3fff;
      block_desc.ptr = reinterpret_cast<char *>(
          arena == nullptr
              ? LowLevelAlloc::Alloc(block_desc.len)
              : LowLevelAlloc::AllocWithArena(block_desc.len, arena));
      using_low_level_alloc = false;
      RandomizeBlockDesc(&block_desc);
      rnd = rand();
      it = allocated.find(rnd);
      if (it != allocated.end()) {
        CheckBlockDesc(it->second);
        using_low_level_alloc = true;
        LowLevelAlloc::Free(it->second.ptr);
        using_low_level_alloc = false;
        it->second = block_desc;
      } else {
        allocated[rnd] = block_desc;
      }
      break;
    case 1:     // coin came up tails: remove a block
      it = allocated.begin();
      if (it != allocated.end()) {
        CheckBlockDesc(it->second);
        using_low_level_alloc = true;
        LowLevelAlloc::Free(it->second.ptr);
        using_low_level_alloc = false;
        allocated.erase(it);
      }
      break;
    }
  }
  // remove all remaining blocks
  while ((it = allocated.begin()) != allocated.end()) {
    CheckBlockDesc(it->second);
    using_low_level_alloc = true;
    LowLevelAlloc::Free(it->second.ptr);
    using_low_level_alloc = false;
    allocated.erase(it);
  }
  if (use_new_arena) {
    TEST_ASSERT(LowLevelAlloc::DeleteArena(arena));
  }
}

// LowLevelAlloc is designed to be safe to call before main().
static struct BeforeMain {
  BeforeMain() {
    Test(false, false, 50000);
    Test(true, false, 50000);
    Test(true, true, 50000);
  }
} before_main;

}  // namespace
}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

int main(int argc, char *argv[]) {
  // The actual test runs in the global constructor of `before_main`.
  printf("PASS\n");
#ifdef __EMSCRIPTEN__
  // clang-format off
// This is JS here. Don't try to format it.
    MAIN_THREAD_EM_ASM({
      if (ENVIRONMENT_IS_WEB) {
        if (typeof TEST_FINISH === 'function') {
          TEST_FINISH($0);
        } else {
          console.error('Attempted to exit with status ' + $0);
          console.error('But TEST_FINSIHED is not a function.');
        }
      }
    }, 0);
// clang-format on
#endif
  return 0;
}
