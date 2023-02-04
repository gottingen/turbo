//
// Copyright 2022 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/log/die_if_null.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "turbo/log/internal/test_helpers.h"
#include "turbo/platform/port.h"
#include "gtest/gtest.h"

namespace {

auto* test_env TURBO_ATTRIBUTE_UNUSED = ::testing::AddGlobalTestEnvironment(
    new turbo::log_internal::LogTestEnvironment);

// TODO(b/69907837): Revisit these tests with the goal of making them less
// convoluted.
TEST(TurboDieIfNull, Simple) {
  int64_t t;
  void* ptr = static_cast<void*>(&t);
  void* ref = TURBO_DIE_IF_NULL(ptr);
  ASSERT_EQ(ptr, ref);

  char* t_as_char;
  t_as_char = TURBO_DIE_IF_NULL(reinterpret_cast<char*>(&t));
  (void)t_as_char;

  unsigned char* t_as_uchar;
  t_as_uchar = TURBO_DIE_IF_NULL(reinterpret_cast<unsigned char*>(&t));
  (void)t_as_uchar;

  int* t_as_int;
  t_as_int = TURBO_DIE_IF_NULL(reinterpret_cast<int*>(&t));
  (void)t_as_int;

  int64_t* t_as_int64_t;
  t_as_int64_t = TURBO_DIE_IF_NULL(reinterpret_cast<int64_t*>(&t));
  (void)t_as_int64_t;

  std::unique_ptr<int64_t> sptr(new int64_t);
  EXPECT_EQ(sptr.get(), TURBO_DIE_IF_NULL(sptr).get());
  TURBO_DIE_IF_NULL(sptr).reset();

  int64_t* int_ptr = new int64_t();
  EXPECT_EQ(int_ptr, TURBO_DIE_IF_NULL(std::unique_ptr<int64_t>(int_ptr)).get());
}

#if GTEST_HAS_DEATH_TEST
TEST(DeathCheckTurboDieIfNull, Simple) {
  void* ptr;
  ASSERT_DEATH({ ptr = TURBO_DIE_IF_NULL(nullptr); }, "");
  (void)ptr;

  std::unique_ptr<int64_t> sptr;
  ASSERT_DEATH(ptr = TURBO_DIE_IF_NULL(sptr).get(), "");
}
#endif

// Ensures that TURBO_DIE_IF_NULL works with C++11's std::unique_ptr and
// std::shared_ptr.
TEST(TurboDieIfNull, DoesNotCompareSmartPointerToNULL) {
  std::unique_ptr<int> up(new int);
  EXPECT_EQ(&up, &TURBO_DIE_IF_NULL(up));
  TURBO_DIE_IF_NULL(up).reset();

  std::shared_ptr<int> sp(new int);
  EXPECT_EQ(&sp, &TURBO_DIE_IF_NULL(sp));
  TURBO_DIE_IF_NULL(sp).reset();
}

// Verifies that TURBO_DIE_IF_NULL returns an rvalue reference if its argument is
// an rvalue reference.
TEST(TurboDieIfNull, PreservesRValues) {
  int64_t* ptr = new int64_t();
  auto uptr = TURBO_DIE_IF_NULL(std::unique_ptr<int64_t>(ptr));
  EXPECT_EQ(ptr, uptr.get());
}

// Verifies that TURBO_DIE_IF_NULL returns an lvalue if its argument is an
// lvalue.
TEST(TurboDieIfNull, PreservesLValues) {
  int64_t array[2] = {0};
  int64_t* a = array + 0;
  int64_t* b = array + 1;
  using std::swap;
  swap(TURBO_DIE_IF_NULL(a), TURBO_DIE_IF_NULL(b));
  EXPECT_EQ(array + 1, a);
  EXPECT_EQ(array + 0, b);
}

}  // namespace
