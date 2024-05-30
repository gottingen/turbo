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

#include <turbo/base/internal/strerror.h>

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/strings/match.h>

namespace {
using ::testing::AnyOf;
using ::testing::Eq;

TEST(StrErrorTest, ValidErrorCode) {
  errno = ERANGE;
  EXPECT_THAT(turbo::base_internal::StrError(EDOM), Eq(strerror(EDOM)));
  EXPECT_THAT(errno, Eq(ERANGE));
}

TEST(StrErrorTest, InvalidErrorCode) {
  errno = ERANGE;
  EXPECT_THAT(turbo::base_internal::StrError(-1),
              AnyOf(Eq("No error information"), Eq("Unknown error -1")));
  EXPECT_THAT(errno, Eq(ERANGE));
}

TEST(StrErrorTest, MultipleThreads) {
  // In this test, we will start up 2 threads and have each one call
  // StrError 1000 times, each time with a different errnum.  We
  // expect that StrError(errnum) will return a string equal to the
  // one returned by strerror(errnum), if the code is known.  Since
  // strerror is known to be thread-hostile, collect all the expected
  // strings up front.
  const int kNumCodes = 1000;
  std::vector<std::string> expected_strings(kNumCodes);
  for (int i = 0; i < kNumCodes; ++i) {
    expected_strings[i] = strerror(i);
  }

  std::atomic_int counter(0);
  auto thread_fun = [&]() {
    for (int i = 0; i < kNumCodes; ++i) {
      ++counter;
      errno = ERANGE;
      const std::string value = turbo::base_internal::StrError(i);
      // EXPECT_* could change errno. Stash it first.
      int check_err = errno;
      EXPECT_THAT(check_err, Eq(ERANGE));
      // Only the GNU implementation is guaranteed to provide the
      // string "Unknown error nnn". POSIX doesn't say anything.
      if (!turbo::starts_with(value, "Unknown error ")) {
        EXPECT_THAT(value, Eq(expected_strings[i]));
      }
    }
  };

  const int kNumThreads = 100;
  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.push_back(std::thread(thread_fun));
  }
  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_THAT(counter, Eq(kNumThreads * kNumCodes));
}

}  // namespace
