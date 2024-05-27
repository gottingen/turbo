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
//
// -----------------------------------------------------------------------------
// File: status_matchers_test.cc
// -----------------------------------------------------------------------------
#include <tests/status/status_matchers_api.h>

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include <turbo/status/status.h>
#include <turbo/status/statusor.h>
#include <turbo/strings/string_view.h>

namespace {

using ::turbo_testing::IsOk;
using ::turbo_testing::IsOkAndHolds;
using ::turbo_testing::StatusIs;
using ::testing::Gt;

TEST(StatusMatcherTest, StatusIsOk) { EXPECT_THAT(turbo::OkStatus(), IsOk()); }

TEST(StatusMatcherTest, StatusOrIsOk) {
  turbo::StatusOr<int> ok_int = {0};
  EXPECT_THAT(ok_int, IsOk());
}

TEST(StatusMatcherTest, StatusIsNotOk) {
  turbo::Status error = turbo::UnknownError("Smigla");
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(error, IsOk()), "Smigla");
}

TEST(StatusMatcherTest, StatusOrIsNotOk) {
  turbo::StatusOr<int> error = turbo::UnknownError("Smigla");
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(error, IsOk()), "Smigla");
}
/*
TEST(StatusMatcherTest, IsOkAndHolds) {
  turbo::StatusOr<int> ok_int = {4};
  turbo::StatusOr<turbo::string_view> ok_str = {"text"};
  EXPECT_THAT(ok_int, IsOkAndHolds(4));
  EXPECT_THAT(ok_int, IsOkAndHolds(Gt(0)));
  EXPECT_THAT(ok_str, IsOkAndHolds("text"));
}

TEST(StatusMatcherTest, IsOkAndHoldsFailure) {
  turbo::StatusOr<int> ok_int = {502};
  turbo::StatusOr<int> error = turbo::UnknownError("Smigla");
  turbo::StatusOr<turbo::string_view> ok_str = {"actual"};
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(ok_int, IsOkAndHolds(0)), "502");
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(error, IsOkAndHolds(0)), "Smigla");
  EXPECT_NONFATAL_FAILURE(EXPECT_THAT(ok_str, IsOkAndHolds("expected")),
                          "actual");
}*/
/*
TEST(StatusMatcherTest, StatusIs) {
  turbo::Status unknown = turbo::UnknownError("unbekannt");
  turbo::Status invalid = turbo::InvalidArgumentError("ungueltig");
  EXPECT_THAT(turbo::OkStatus(), StatusIs(turbo::StatusCode::kOk));
  EXPECT_THAT(turbo::OkStatus(), StatusIs(0));
  EXPECT_THAT(unknown, StatusIs(turbo::StatusCode::kUnknown));
  EXPECT_THAT(unknown, StatusIs(2));
  EXPECT_THAT(unknown, StatusIs(turbo::StatusCode::kUnknown, "unbekannt"));
  EXPECT_THAT(invalid, StatusIs(turbo::StatusCode::kInvalidArgument));
  EXPECT_THAT(invalid, StatusIs(3));
  EXPECT_THAT(invalid,
              StatusIs(turbo::StatusCode::kInvalidArgument, "ungueltig"));
}

TEST(StatusMatcherTest, StatusOrIs) {
  turbo::StatusOr<int> ok = {42};
  turbo::StatusOr<int> unknown = turbo::UnknownError("unbekannt");
  turbo::StatusOr<turbo::string_view> invalid =
      turbo::InvalidArgumentError("ungueltig");
  EXPECT_THAT(ok, StatusIs(turbo::StatusCode::kOk));
  EXPECT_THAT(ok, StatusIs(0));
  EXPECT_THAT(unknown, StatusIs(turbo::StatusCode::kUnknown));
  EXPECT_THAT(unknown, StatusIs(2));
  EXPECT_THAT(unknown, StatusIs(turbo::StatusCode::kUnknown, "unbekannt"));
  EXPECT_THAT(invalid, StatusIs(turbo::StatusCode::kInvalidArgument));
  EXPECT_THAT(invalid, StatusIs(3));
  EXPECT_THAT(invalid,
              StatusIs(turbo::StatusCode::kInvalidArgument, "ungueltig"));
}

TEST(StatusMatcherTest, StatusIsFailure) {
  turbo::Status unknown = turbo::UnknownError("unbekannt");
  turbo::Status invalid = turbo::InvalidArgumentError("ungueltig");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(turbo::OkStatus(),
                  StatusIs(turbo::StatusCode::kInvalidArgument)),
      "OK");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(unknown, StatusIs(turbo::StatusCode::kCancelled)), "UNKNOWN");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(unknown, StatusIs(turbo::StatusCode::kUnknown, "inconnu")),
      "unbekannt");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(invalid, StatusIs(turbo::StatusCode::kOutOfRange)), "INVALID");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THAT(invalid,
                  StatusIs(turbo::StatusCode::kInvalidArgument, "invalide")),
      "ungueltig");
}*/

}  // namespace
