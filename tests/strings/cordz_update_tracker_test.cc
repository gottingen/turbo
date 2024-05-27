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

#include <turbo/strings/internal/cordz_update_tracker.h>

#include <array>
#include <thread>  // NOLINT

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/synchronization/notification.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {
namespace {

using ::testing::AnyOf;
using ::testing::Eq;

using Method = CordzUpdateTracker::MethodIdentifier;
using Methods = std::array<Method, Method::kNumMethods>;

// Returns an array of all methods defined in `MethodIdentifier`
Methods AllMethods() {
  return Methods{Method::kUnknown,
                 Method::kAppendCord,
                 Method::kAppendCordBuffer,
                 Method::kAppendExternalMemory,
                 Method::kAppendString,
                 Method::kAssignCord,
                 Method::kAssignString,
                 Method::kClear,
                 Method::kConstructorCord,
                 Method::kConstructorString,
                 Method::kCordReader,
                 Method::kFlatten,
                 Method::kGetAppendBuffer,
                 Method::kGetAppendRegion,
                 Method::kMakeCordFromExternal,
                 Method::kMoveAppendCord,
                 Method::kMoveAssignCord,
                 Method::kMovePrependCord,
                 Method::kPrependCord,
                 Method::kPrependCordBuffer,
                 Method::kPrependString,
                 Method::kRemovePrefix,
                 Method::kRemoveSuffix,
                 Method::kSetExpectedChecksum,
                 Method::kSubCord};
}

TEST(CordzUpdateTracker, IsConstExprAndInitializesToZero) {
  constexpr CordzUpdateTracker tracker;
  for (Method method : AllMethods()) {
    ASSERT_THAT(tracker.Value(method), Eq(0));
  }
}

TEST(CordzUpdateTracker, LossyAdd) {
  int64_t n = 1;
  CordzUpdateTracker tracker;
  for (Method method : AllMethods()) {
    tracker.LossyAdd(method, n);
    EXPECT_THAT(tracker.Value(method), Eq(n));
    n += 2;
  }
}

TEST(CordzUpdateTracker, CopyConstructor) {
  int64_t n = 1;
  CordzUpdateTracker src;
  for (Method method : AllMethods()) {
    src.LossyAdd(method, n);
    n += 2;
  }

  n = 1;
  CordzUpdateTracker tracker(src);
  for (Method method : AllMethods()) {
    EXPECT_THAT(tracker.Value(method), Eq(n));
    n += 2;
  }
}

TEST(CordzUpdateTracker, OperatorAssign) {
  int64_t n = 1;
  CordzUpdateTracker src;
  CordzUpdateTracker tracker;
  for (Method method : AllMethods()) {
    src.LossyAdd(method, n);
    n += 2;
  }

  n = 1;
  tracker = src;
  for (Method method : AllMethods()) {
    EXPECT_THAT(tracker.Value(method), Eq(n));
    n += 2;
  }
}

TEST(CordzUpdateTracker, ThreadSanitizedValueCheck) {
  turbo::Notification done;
  CordzUpdateTracker tracker;

  std::thread reader([&done, &tracker] {
    while (!done.HasBeenNotified()) {
      int n = 1;
      for (Method method : AllMethods()) {
        EXPECT_THAT(tracker.Value(method), AnyOf(Eq(n), Eq(0)));
        n += 2;
      }
    }
    int n = 1;
    for (Method method : AllMethods()) {
      EXPECT_THAT(tracker.Value(method), Eq(n));
      n += 2;
    }
  });

  int64_t n = 1;
  for (Method method : AllMethods()) {
    tracker.LossyAdd(method, n);
    n += 2;
  }
  done.Notify();
  reader.join();
}

}  // namespace
}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
