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

#include <turbo/base/nullability.h>

#include <cassert>
#include <memory>
#include <utility>

#include <gtest/gtest.h>
#include <turbo/base/attributes.h>

namespace {
using ::turbo::Nonnull;
using ::turbo::NullabilityUnknown;
using ::turbo::Nullable;

void funcWithNonnullArg(Nonnull<int*> /*arg*/) {}
template <typename T>
void funcWithDeducedNonnullArg(Nonnull<T*> /*arg*/) {}

TEST(NonnullTest, NonnullArgument) {
  int var = 0;
  funcWithNonnullArg(&var);
  funcWithDeducedNonnullArg(&var);
}

Nonnull<int*> funcWithNonnullReturn() {
  static int var = 0;
  return &var;
}

TEST(NonnullTest, NonnullReturn) {
  auto var = funcWithNonnullReturn();
  (void)var;
}

TEST(PassThroughTest, PassesThroughRawPointerToInt) {
  EXPECT_TRUE((std::is_same<Nonnull<int*>, int*>::value));
  EXPECT_TRUE((std::is_same<Nullable<int*>, int*>::value));
  EXPECT_TRUE((std::is_same<NullabilityUnknown<int*>, int*>::value));
}

TEST(PassThroughTest, PassesThroughRawPointerToVoid) {
  EXPECT_TRUE((std::is_same<Nonnull<void*>, void*>::value));
  EXPECT_TRUE((std::is_same<Nullable<void*>, void*>::value));
  EXPECT_TRUE((std::is_same<NullabilityUnknown<void*>, void*>::value));
}

TEST(PassThroughTest, PassesThroughUniquePointerToInt) {
  using T = std::unique_ptr<int>;
  EXPECT_TRUE((std::is_same<Nonnull<T>, T>::value));
  EXPECT_TRUE((std::is_same<Nullable<T>, T>::value));
  EXPECT_TRUE((std::is_same<NullabilityUnknown<T>, T>::value));
}

TEST(PassThroughTest, PassesThroughSharedPointerToInt) {
  using T = std::shared_ptr<int>;
  EXPECT_TRUE((std::is_same<Nonnull<T>, T>::value));
  EXPECT_TRUE((std::is_same<Nullable<T>, T>::value));
  EXPECT_TRUE((std::is_same<NullabilityUnknown<T>, T>::value));
}

TEST(PassThroughTest, PassesThroughSharedPointerToVoid) {
  using T = std::shared_ptr<void>;
  EXPECT_TRUE((std::is_same<Nonnull<T>, T>::value));
  EXPECT_TRUE((std::is_same<Nullable<T>, T>::value));
  EXPECT_TRUE((std::is_same<NullabilityUnknown<T>, T>::value));
}

TEST(PassThroughTest, PassesThroughPointerToMemberObject) {
  using T = decltype(&std::pair<int, int>::first);
  EXPECT_TRUE((std::is_same<Nonnull<T>, T>::value));
  EXPECT_TRUE((std::is_same<Nullable<T>, T>::value));
  EXPECT_TRUE((std::is_same<NullabilityUnknown<T>, T>::value));
}

TEST(PassThroughTest, PassesThroughPointerToMemberFunction) {
  using T = decltype(&std::unique_ptr<int>::reset);
  EXPECT_TRUE((std::is_same<Nonnull<T>, T>::value));
  EXPECT_TRUE((std::is_same<Nullable<T>, T>::value));
  EXPECT_TRUE((std::is_same<NullabilityUnknown<T>, T>::value));
}

}  // namespace

// Nullable ADL lookup test
namespace util {
// Helper for NullableAdlTest.  Returns true, denoting that argument-dependent
// lookup found this implementation of DidAdlWin.  Must be in namespace
// util itself, not a nested anonymous namespace.
template <typename T>
bool DidAdlWin(T*) {
  return true;
}

// Because this type is defined in namespace util, an unqualified call to
// DidAdlWin with a pointer to MakeAdlWin will find the above implementation.
struct MakeAdlWin {};
}  // namespace util

namespace {
// Returns false, denoting that ADL did not inspect namespace util.  If it
// had, the better match (T*) above would have won out over the (...) here.
bool DidAdlWin(...) { return false; }

TEST(NullableAdlTest, NullableAddsNothingToArgumentDependentLookup) {
  // Treatment: util::Nullable<int*> contributes nothing to ADL because
  // int* itself doesn't.
  EXPECT_FALSE(DidAdlWin((int*)nullptr));
  EXPECT_FALSE(DidAdlWin((Nullable<int*>)nullptr));

  // Control: Argument-dependent lookup does find the implementation in
  // namespace util when the underlying pointee type resides there.
  EXPECT_TRUE(DidAdlWin((util::MakeAdlWin*)nullptr));
  EXPECT_TRUE(DidAdlWin((Nullable<util::MakeAdlWin*>)nullptr));
}
}  // namespace
