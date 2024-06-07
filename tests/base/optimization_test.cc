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

#include <turbo/base/optimization.h>

#include <gtest/gtest.h>
#include <optional>

namespace {

// Tests for the TURBO_LIKELY and TURBO_UNLIKELY macros.
// The tests only verify that the macros are functionally correct - i.e. code
// behaves as if they weren't used. They don't try to check their impact on
// optimization.

TEST(PredictTest, PredictTrue) {
  EXPECT_TRUE(TURBO_LIKELY(true));
  EXPECT_FALSE(TURBO_LIKELY(false));
  EXPECT_TRUE(TURBO_LIKELY(1 == 1));
  EXPECT_FALSE(TURBO_LIKELY(1 == 2));

  if (TURBO_LIKELY(false)) ADD_FAILURE();
  if (!TURBO_LIKELY(true)) ADD_FAILURE();

  EXPECT_TRUE(TURBO_LIKELY(true) && true);
  EXPECT_TRUE(TURBO_LIKELY(true) || false);
}

TEST(PredictTest, PredictFalse) {
  EXPECT_TRUE(TURBO_UNLIKELY(true));
  EXPECT_FALSE(TURBO_UNLIKELY(false));
  EXPECT_TRUE(TURBO_UNLIKELY(1 == 1));
  EXPECT_FALSE(TURBO_UNLIKELY(1 == 2));

  if (TURBO_UNLIKELY(false)) ADD_FAILURE();
  if (!TURBO_UNLIKELY(true)) ADD_FAILURE();

  EXPECT_TRUE(TURBO_UNLIKELY(true) && true);
  EXPECT_TRUE(TURBO_UNLIKELY(true) || false);
}

TEST(PredictTest, OneEvaluation) {
  // Verify that the expression is only evaluated once.
  int x = 0;
  if (TURBO_LIKELY((++x) == 0)) ADD_FAILURE();
  EXPECT_EQ(x, 1);
  if (TURBO_UNLIKELY((++x) == 0)) ADD_FAILURE();
  EXPECT_EQ(x, 2);
}

TEST(PredictTest, OperatorOrder) {
  // Verify that operator order inside and outside the macro behaves well.
  // These would fail for a naive '#define TURBO_LIKELY(x) x'
  EXPECT_TRUE(TURBO_LIKELY(1 && 2) == true);
  EXPECT_TRUE(TURBO_UNLIKELY(1 && 2) == true);
  EXPECT_TRUE(!TURBO_LIKELY(1 == 2));
  EXPECT_TRUE(!TURBO_UNLIKELY(1 == 2));
}

TEST(PredictTest, Pointer) {
  const int x = 3;
  const int *good_intptr = &x;
  const int *null_intptr = nullptr;
  EXPECT_TRUE(TURBO_LIKELY(good_intptr));
  EXPECT_FALSE(TURBO_LIKELY(null_intptr));
  EXPECT_TRUE(TURBO_UNLIKELY(good_intptr));
  EXPECT_FALSE(TURBO_UNLIKELY(null_intptr));
}

TEST(PredictTest, Optional) {
  // Note: An optional's truth value is the value's existence, not its truth.
  std::optional<bool> has_value(false);
  std::optional<bool> no_value;
  EXPECT_TRUE(TURBO_LIKELY(has_value));
  EXPECT_FALSE(TURBO_LIKELY(no_value));
  EXPECT_TRUE(TURBO_UNLIKELY(has_value));
  EXPECT_FALSE(TURBO_UNLIKELY(no_value));
}

class ImplictlyConvertibleToBool {
 public:
  explicit ImplictlyConvertibleToBool(bool value) : value_(value) {}
  operator bool() const {  // NOLINT(google-explicit-constructor)
    return value_;
  }

 private:
  bool value_;
};

TEST(PredictTest, ImplicitBoolConversion) {
  const ImplictlyConvertibleToBool is_true(true);
  const ImplictlyConvertibleToBool is_false(false);
  if (!TURBO_LIKELY(is_true)) ADD_FAILURE();
  if (TURBO_LIKELY(is_false)) ADD_FAILURE();
  if (!TURBO_UNLIKELY(is_true)) ADD_FAILURE();
  if (TURBO_UNLIKELY(is_false)) ADD_FAILURE();
}

class ExplictlyConvertibleToBool {
 public:
  explicit ExplictlyConvertibleToBool(bool value) : value_(value) {}
  explicit operator bool() const { return value_; }

 private:
  bool value_;
};

TEST(PredictTest, ExplicitBoolConversion) {
  const ExplictlyConvertibleToBool is_true(true);
  const ExplictlyConvertibleToBool is_false(false);
  if (!TURBO_LIKELY(is_true)) ADD_FAILURE();
  if (TURBO_LIKELY(is_false)) ADD_FAILURE();
  if (!TURBO_UNLIKELY(is_true)) ADD_FAILURE();
  if (TURBO_UNLIKELY(is_false)) ADD_FAILURE();
}

}  // namespace
