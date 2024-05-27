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
#include <turbo/types/optional.h>

namespace {

// Tests for the TURBO_PREDICT_TRUE and TURBO_PREDICT_FALSE macros.
// The tests only verify that the macros are functionally correct - i.e. code
// behaves as if they weren't used. They don't try to check their impact on
// optimization.

TEST(PredictTest, PredictTrue) {
  EXPECT_TRUE(TURBO_PREDICT_TRUE(true));
  EXPECT_FALSE(TURBO_PREDICT_TRUE(false));
  EXPECT_TRUE(TURBO_PREDICT_TRUE(1 == 1));
  EXPECT_FALSE(TURBO_PREDICT_TRUE(1 == 2));

  if (TURBO_PREDICT_TRUE(false)) ADD_FAILURE();
  if (!TURBO_PREDICT_TRUE(true)) ADD_FAILURE();

  EXPECT_TRUE(TURBO_PREDICT_TRUE(true) && true);
  EXPECT_TRUE(TURBO_PREDICT_TRUE(true) || false);
}

TEST(PredictTest, PredictFalse) {
  EXPECT_TRUE(TURBO_PREDICT_FALSE(true));
  EXPECT_FALSE(TURBO_PREDICT_FALSE(false));
  EXPECT_TRUE(TURBO_PREDICT_FALSE(1 == 1));
  EXPECT_FALSE(TURBO_PREDICT_FALSE(1 == 2));

  if (TURBO_PREDICT_FALSE(false)) ADD_FAILURE();
  if (!TURBO_PREDICT_FALSE(true)) ADD_FAILURE();

  EXPECT_TRUE(TURBO_PREDICT_FALSE(true) && true);
  EXPECT_TRUE(TURBO_PREDICT_FALSE(true) || false);
}

TEST(PredictTest, OneEvaluation) {
  // Verify that the expression is only evaluated once.
  int x = 0;
  if (TURBO_PREDICT_TRUE((++x) == 0)) ADD_FAILURE();
  EXPECT_EQ(x, 1);
  if (TURBO_PREDICT_FALSE((++x) == 0)) ADD_FAILURE();
  EXPECT_EQ(x, 2);
}

TEST(PredictTest, OperatorOrder) {
  // Verify that operator order inside and outside the macro behaves well.
  // These would fail for a naive '#define TURBO_PREDICT_TRUE(x) x'
  EXPECT_TRUE(TURBO_PREDICT_TRUE(1 && 2) == true);
  EXPECT_TRUE(TURBO_PREDICT_FALSE(1 && 2) == true);
  EXPECT_TRUE(!TURBO_PREDICT_TRUE(1 == 2));
  EXPECT_TRUE(!TURBO_PREDICT_FALSE(1 == 2));
}

TEST(PredictTest, Pointer) {
  const int x = 3;
  const int *good_intptr = &x;
  const int *null_intptr = nullptr;
  EXPECT_TRUE(TURBO_PREDICT_TRUE(good_intptr));
  EXPECT_FALSE(TURBO_PREDICT_TRUE(null_intptr));
  EXPECT_TRUE(TURBO_PREDICT_FALSE(good_intptr));
  EXPECT_FALSE(TURBO_PREDICT_FALSE(null_intptr));
}

TEST(PredictTest, Optional) {
  // Note: An optional's truth value is the value's existence, not its truth.
  turbo::optional<bool> has_value(false);
  turbo::optional<bool> no_value;
  EXPECT_TRUE(TURBO_PREDICT_TRUE(has_value));
  EXPECT_FALSE(TURBO_PREDICT_TRUE(no_value));
  EXPECT_TRUE(TURBO_PREDICT_FALSE(has_value));
  EXPECT_FALSE(TURBO_PREDICT_FALSE(no_value));
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
  if (!TURBO_PREDICT_TRUE(is_true)) ADD_FAILURE();
  if (TURBO_PREDICT_TRUE(is_false)) ADD_FAILURE();
  if (!TURBO_PREDICT_FALSE(is_true)) ADD_FAILURE();
  if (TURBO_PREDICT_FALSE(is_false)) ADD_FAILURE();
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
  if (!TURBO_PREDICT_TRUE(is_true)) ADD_FAILURE();
  if (TURBO_PREDICT_TRUE(is_false)) ADD_FAILURE();
  if (!TURBO_PREDICT_FALSE(is_true)) ADD_FAILURE();
  if (TURBO_PREDICT_FALSE(is_false)) ADD_FAILURE();
}

}  // namespace
