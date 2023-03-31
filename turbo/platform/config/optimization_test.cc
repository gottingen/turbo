// Copyright 2020 The Turbo Authors.
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

#include "attribute_optimization.h"

#include "gtest/gtest.h"
#include "turbo/meta/optional.h"

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
  turbo::optional<bool> has_value(false);
  turbo::optional<bool> no_value;
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
