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

#include "turbo/platform/internal/atomic_hook.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/platform/attributes.h"
#include "turbo/platform/internal/atomic_hook_test_helper.h"

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
