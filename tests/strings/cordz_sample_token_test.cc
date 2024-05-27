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

#include <turbo/strings/internal/cordz_sample_token.h>

#include <memory>
#include <type_traits>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/memory/memory.h>
#include <turbo/random/random.h>
#include <tests/strings/cordz_test_helpers.h>
#include <turbo/strings/internal/cord_rep_flat.h>
#include <turbo/strings/internal/cordz_handle.h>
#include <turbo/strings/internal/cordz_info.h>
#include <turbo/synchronization/internal/thread_pool.h>
#include <turbo/synchronization/notification.h>
#include <turbo/time/clock.h>
#include <turbo/time/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {
namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Ne;

// Used test values
auto constexpr kTrackCordMethod = CordzUpdateTracker::kConstructorString;

TEST(CordzSampleTokenTest, IteratorTraits) {
  static_assert(std::is_copy_constructible<CordzSampleToken::Iterator>::value,
                "");
  static_assert(std::is_copy_assignable<CordzSampleToken::Iterator>::value, "");
  static_assert(std::is_move_constructible<CordzSampleToken::Iterator>::value,
                "");
  static_assert(std::is_move_assignable<CordzSampleToken::Iterator>::value, "");
  static_assert(
      std::is_same<
          std::iterator_traits<CordzSampleToken::Iterator>::iterator_category,
          std::input_iterator_tag>::value,
      "");
  static_assert(
      std::is_same<std::iterator_traits<CordzSampleToken::Iterator>::value_type,
                   const CordzInfo&>::value,
      "");
  static_assert(
      std::is_same<
          std::iterator_traits<CordzSampleToken::Iterator>::difference_type,
          ptrdiff_t>::value,
      "");
  static_assert(
      std::is_same<std::iterator_traits<CordzSampleToken::Iterator>::pointer,
                   const CordzInfo*>::value,
      "");
  static_assert(
      std::is_same<std::iterator_traits<CordzSampleToken::Iterator>::reference,
                   const CordzInfo&>::value,
      "");
}

TEST(CordzSampleTokenTest, IteratorEmpty) {
  CordzSampleToken token;
  EXPECT_THAT(token.begin(), Eq(token.end()));
}

TEST(CordzSampleTokenTest, Iterator) {
  TestCordData cord1, cord2, cord3;
  CordzInfo::TrackCord(cord1.data, kTrackCordMethod, 1);
  CordzInfo* info1 = cord1.data.cordz_info();
  CordzInfo::TrackCord(cord2.data, kTrackCordMethod, 1);
  CordzInfo* info2 = cord2.data.cordz_info();
  CordzInfo::TrackCord(cord3.data, kTrackCordMethod, 1);
  CordzInfo* info3 = cord3.data.cordz_info();

  CordzSampleToken token;
  std::vector<const CordzInfo*> found;
  for (const CordzInfo& cord_info : token) {
    found.push_back(&cord_info);
  }

  EXPECT_THAT(found, ElementsAre(info3, info2, info1));

  info1->Untrack();
  info2->Untrack();
  info3->Untrack();
}

TEST(CordzSampleTokenTest, IteratorEquality) {
  TestCordData cord1;
  TestCordData cord2;
  TestCordData cord3;
  CordzInfo::TrackCord(cord1.data, kTrackCordMethod, 1);
  CordzInfo* info1 = cord1.data.cordz_info();

  CordzSampleToken token1;
  // lhs starts with the CordzInfo corresponding to cord1 at the head.
  CordzSampleToken::Iterator lhs = token1.begin();

  CordzInfo::TrackCord(cord2.data, kTrackCordMethod, 1);
  CordzInfo* info2 = cord2.data.cordz_info();

  CordzSampleToken token2;
  // rhs starts with the CordzInfo corresponding to cord2 at the head.
  CordzSampleToken::Iterator rhs = token2.begin();

  CordzInfo::TrackCord(cord3.data, kTrackCordMethod, 1);
  CordzInfo* info3 = cord3.data.cordz_info();

  // lhs is on cord1 while rhs is on cord2.
  EXPECT_THAT(lhs, Ne(rhs));

  rhs++;
  // lhs and rhs are both on cord1, but they didn't come from the same
  // CordzSampleToken.
  EXPECT_THAT(lhs, Ne(rhs));

  lhs++;
  rhs++;
  // Both lhs and rhs are done, so they are on nullptr.
  EXPECT_THAT(lhs, Eq(rhs));

  info1->Untrack();
  info2->Untrack();
  info3->Untrack();
}

TEST(CordzSampleTokenTest, MultiThreaded) {
  Notification stop;
  static constexpr int kNumThreads = 4;
  static constexpr int kNumCords = 3;
  static constexpr int kNumTokens = 3;
  turbo::synchronization_internal::ThreadPool pool(kNumThreads);

  for (int i = 0; i < kNumThreads; ++i) {
    pool.Schedule([&stop]() {
      turbo::BitGen gen;
      TestCordData cords[kNumCords];
      std::unique_ptr<CordzSampleToken> tokens[kNumTokens];

      while (!stop.HasBeenNotified()) {
        // Randomly perform one of five actions:
        //   1) Untrack
        //   2) Track
        //   3) Iterate over Cords visible to a token.
        //   4) Unsample
        //   5) Sample
        int index = turbo::Uniform(gen, 0, kNumCords);
        if (turbo::Bernoulli(gen, 0.5)) {
          TestCordData& cord = cords[index];
          // Track/untrack.
          if (cord.data.is_profiled()) {
            // 1) Untrack
            cord.data.cordz_info()->Untrack();
            cord.data.clear_cordz_info();
          } else {
            // 2) Track
            CordzInfo::TrackCord(cord.data, kTrackCordMethod, 1);
          }
        } else {
          std::unique_ptr<CordzSampleToken>& token = tokens[index];
          if (token) {
            if (turbo::Bernoulli(gen, 0.5)) {
              // 3) Iterate over Cords visible to a token.
              for (const CordzInfo& info : *token) {
                // This is trivial work to allow us to compile the loop.
                EXPECT_THAT(info.Next(*token), Ne(&info));
              }
            } else {
              // 4) Unsample
              token = nullptr;
            }
          } else {
            // 5) Sample
            token = turbo::make_unique<CordzSampleToken>();
          }
        }
      }
      for (TestCordData& cord : cords) {
        CordzInfo::MaybeUntrackCord(cord.data.cordz_info());
      }
    });
  }
  // The threads will hammer away.  Give it a little bit of time for tsan to
  // spot errors.
  turbo::SleepFor(turbo::Seconds(3));
  stop.Notify();
}

}  // namespace
}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
