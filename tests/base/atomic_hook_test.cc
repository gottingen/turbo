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

#include <turbo/base/internal/atomic_hook.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <tests/base/atomic_hook_test_helper.h>

namespace {

using ::testing::Eq;

int value = 0;
void TestHook(int x) { value = x; }

TEST(AtomicHookTest, NoDefaultFunction) {
  TURBO_INTERNAL_ATOMIC_HOOK_ATTRIBUTES static turbo::base_internal::AtomicHook<
      void (*)(int)>
      hook;
  value = 0;

  // Test the default DummyFunction.
  EXPECT_TRUE(hook.Load() == nullptr);
  EXPECT_EQ(value, 0);
  hook(1);
  EXPECT_EQ(value, 0);

  // Test a stored hook.
  hook.Store(TestHook);
  EXPECT_TRUE(hook.Load() == TestHook);
  EXPECT_EQ(value, 0);
  hook(1);
  EXPECT_EQ(value, 1);

  // Calling Store() with the same hook should not crash.
  hook.Store(TestHook);
  EXPECT_TRUE(hook.Load() == TestHook);
  EXPECT_EQ(value, 1);
  hook(2);
  EXPECT_EQ(value, 2);
}

TEST(AtomicHookTest, WithDefaultFunction) {
  // Set the default value to TestHook at compile-time.
  TURBO_INTERNAL_ATOMIC_HOOK_ATTRIBUTES static turbo::base_internal::AtomicHook<
      void (*)(int)>
      hook(TestHook);
  value = 0;

  // Test the default value is TestHook.
  EXPECT_TRUE(hook.Load() == TestHook);
  EXPECT_EQ(value, 0);
  hook(1);
  EXPECT_EQ(value, 1);

  // Calling Store() with the same hook should not crash.
  hook.Store(TestHook);
  EXPECT_TRUE(hook.Load() == TestHook);
  EXPECT_EQ(value, 1);
  hook(2);
  EXPECT_EQ(value, 2);
}

TURBO_CONST_INIT int override_func_calls = 0;
void OverrideFunc() { override_func_calls++; }
static struct OverrideInstaller {
  OverrideInstaller() { turbo::atomic_hook_internal::func.Store(OverrideFunc); }
} override_installer;

TEST(AtomicHookTest, DynamicInitFromAnotherTU) {
  // MSVC 14.2 doesn't do constexpr static init correctly; in particular it
  // tends to sequence static init (i.e. defaults) of `AtomicHook` objects
  // after their dynamic init (i.e. overrides), overwriting whatever value was
  // written during dynamic init.  This regression test validates the fix.
  // https://developercommunity.visualstudio.com/content/problem/336946/class-with-constexpr-constructor-not-using-static.html
  EXPECT_THAT(turbo::atomic_hook_internal::default_func_calls, Eq(0));
  EXPECT_THAT(override_func_calls, Eq(0));
  turbo::atomic_hook_internal::func();
  EXPECT_THAT(turbo::atomic_hook_internal::default_func_calls, Eq(0));
  EXPECT_THAT(override_func_calls, Eq(1));
  EXPECT_THAT(turbo::atomic_hook_internal::func.Load(), Eq(OverrideFunc));
}

}  // namespace
