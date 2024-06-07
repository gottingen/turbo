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

#include <turbo/utility/status_impl.h>

#include <errno.h>

#include <array>
#include <cstddef>
#include <sstream>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/strings/cord.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_format.h>

namespace {

using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Optional;
using ::testing::UnorderedElementsAreArray;

TEST(StatusCode, InsertionOperator) {
  const turbo::StatusCode code = turbo::StatusCode::kUnknown;
  std::ostringstream oss;
  oss << code;
  EXPECT_EQ(oss.str(), turbo::StatusCodeToString(code));
}

// This structure holds the details for testing a single error code,
// its creator, and its classifier.
struct ErrorTest {
  turbo::StatusCode code;
  using Creator = turbo::Status (*)(
      std::string_view
  );
  using Classifier = bool (*)(const turbo::Status&);
  Creator creator;
  Classifier classifier;
};

constexpr ErrorTest kErrorTests[]{
    {turbo::StatusCode::kCancelled, turbo::cancelled_error, turbo::is_cancelled},
    {turbo::StatusCode::kUnknown, turbo::unknown_error, turbo::is_unknown},
    {turbo::StatusCode::kInvalidArgument, turbo::invalid_argument_error,
     turbo::is_invalid_argument},
    {turbo::StatusCode::kDeadlineExceeded, turbo::deadline_exceeded_error,
     turbo::is_deadline_exceeded},
    {turbo::StatusCode::kNotFound, turbo::not_found_error, turbo::is_not_found},
    {turbo::StatusCode::kAlreadyExists, turbo::already_exists_error,
     turbo::is_already_exists},
    {turbo::StatusCode::kPermissionDenied, turbo::permission_denied_error,
     turbo::is_permission_denied},
    {turbo::StatusCode::kResourceExhausted, turbo::resource_exhausted_error,
     turbo::is_resource_exhausted},
    {turbo::StatusCode::kFailedPrecondition, turbo::failed_precondition_error,
     turbo::is_failed_precondition},
    {turbo::StatusCode::kAborted, turbo::aborted_error, turbo::is_aborted},
    {turbo::StatusCode::kOutOfRange, turbo::out_of_range_error, turbo::is_out_of_range},
    {turbo::StatusCode::kUnimplemented, turbo::unimplemented_error,
     turbo::is_unimplemented},
    {turbo::StatusCode::kInternal, turbo::internal_error, turbo::is_internal},
    {turbo::StatusCode::kUnavailable, turbo::unavailable_error,
     turbo::is_unavailable},
    {turbo::StatusCode::kDataLoss, turbo::data_loss_error, turbo::is_data_loss},
    {turbo::StatusCode::kUnauthenticated, turbo::unauthenticated_error,
     turbo::is_unauthenticated},
};

TEST(Status, CreateAndClassify) {
  for (const auto& test : kErrorTests) {
    SCOPED_TRACE(turbo::StatusCodeToString(test.code));

    // Ensure that the creator does, in fact, create status objects with the
    // expected error code and message.
    std::string message =
        turbo::str_cat("error code ", test.code, " test message");
    turbo::Status status = test.creator(
        message
    );
    EXPECT_EQ(test.code, status.code());
    EXPECT_EQ(message, status.message());

    // Ensure that the classifier returns true for a status produced by the
    // creator.
    EXPECT_TRUE(test.classifier(status));

    // Ensure that the classifier returns false for status with a different
    // code.
    for (const auto& other : kErrorTests) {
      if (other.code != test.code) {
        EXPECT_FALSE(test.classifier(turbo::Status(other.code, "")))
            << " other.code = " << other.code;
      }
    }
  }
}

TEST(Status, DefaultConstructor) {
  turbo::Status status;
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(turbo::StatusCode::kOk, status.code());
  EXPECT_EQ("", status.message());
}

TEST(Status, OkStatus) {
  turbo::Status status = turbo::OkStatus();
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(turbo::StatusCode::kOk, status.code());
  EXPECT_EQ("", status.message());
}

TEST(Status, ConstructorWithCodeMessage) {
  {
    turbo::Status status(turbo::StatusCode::kCancelled, "");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(turbo::StatusCode::kCancelled, status.code());
    EXPECT_EQ("", status.message());
  }
  {
    turbo::Status status(turbo::StatusCode::kInternal, "message");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(turbo::StatusCode::kInternal, status.code());
    EXPECT_EQ("message", status.message());
  }
}

TEST(Status, StatusMessageCStringTest) {
  {
    turbo::Status status = turbo::OkStatus();
    EXPECT_EQ(status.message(), "");
    EXPECT_STREQ(turbo::StatusMessageAsCStr(status), "");
    EXPECT_EQ(status.message(), turbo::StatusMessageAsCStr(status));
    EXPECT_NE(turbo::StatusMessageAsCStr(status), nullptr);
  }
  {
    turbo::Status status;
    EXPECT_EQ(status.message(), "");
    EXPECT_NE(turbo::StatusMessageAsCStr(status), nullptr);
    EXPECT_STREQ(turbo::StatusMessageAsCStr(status), "");
  }
  {
    turbo::Status status(turbo::StatusCode::kInternal, "message");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(turbo::StatusCode::kInternal, status.code());
    EXPECT_EQ("message", status.message());
    EXPECT_STREQ("message", turbo::StatusMessageAsCStr(status));
  }
}

TEST(Status, ConstructOutOfRangeCode) {
  const int kRawCode = 9999;
  turbo::Status status(static_cast<turbo::StatusCode>(kRawCode), "");
  EXPECT_EQ(turbo::StatusCode::kUnknown, status.code());
  EXPECT_EQ(kRawCode, status.raw_code());
}

constexpr char kUrl1[] = "url.payload.1";
constexpr char kUrl2[] = "url.payload.2";
constexpr char kUrl3[] = "url.payload.3";
constexpr char kUrl4[] = "url.payload.xx";

constexpr char kPayload1[] = "aaaaa";
constexpr char kPayload2[] = "bbbbb";
constexpr char kPayload3[] = "ccccc";

using PayloadsVec = std::vector<std::pair<std::string, turbo::Cord>>;

TEST(Status, TestGetSetPayload) {
  turbo::Status ok_status = turbo::OkStatus();
  ok_status.set_payload(kUrl1, turbo::Cord(kPayload1));
  ok_status.set_payload(kUrl2, turbo::Cord(kPayload2));

  EXPECT_FALSE(ok_status.get_payload(kUrl1));
  EXPECT_FALSE(ok_status.get_payload(kUrl2));

  turbo::Status bad_status(turbo::StatusCode::kInternal, "fail");
  bad_status.set_payload(kUrl1, turbo::Cord(kPayload1));
  bad_status.set_payload(kUrl2, turbo::Cord(kPayload2));

  EXPECT_THAT(bad_status.get_payload(kUrl1), Optional(Eq(kPayload1)));
  EXPECT_THAT(bad_status.get_payload(kUrl2), Optional(Eq(kPayload2)));

  EXPECT_FALSE(bad_status.get_payload(kUrl3));

  bad_status.set_payload(kUrl1, turbo::Cord(kPayload3));
  EXPECT_THAT(bad_status.get_payload(kUrl1), Optional(Eq(kPayload3)));

  // Testing dynamically generated type_url
  bad_status.set_payload(turbo::str_cat(kUrl1, ".1"), turbo::Cord(kPayload1));
  EXPECT_THAT(bad_status.get_payload(turbo::str_cat(kUrl1, ".1")),
              Optional(Eq(kPayload1)));
}

TEST(Status, TestErasePayload) {
  turbo::Status bad_status(turbo::StatusCode::kInternal, "fail");
  bad_status.set_payload(kUrl1, turbo::Cord(kPayload1));
  bad_status.set_payload(kUrl2, turbo::Cord(kPayload2));
  bad_status.set_payload(kUrl3, turbo::Cord(kPayload3));

  EXPECT_FALSE(bad_status.erase_payload(kUrl4));

  EXPECT_TRUE(bad_status.get_payload(kUrl2));
  EXPECT_TRUE(bad_status.erase_payload(kUrl2));
  EXPECT_FALSE(bad_status.get_payload(kUrl2));
  EXPECT_FALSE(bad_status.erase_payload(kUrl2));

  EXPECT_TRUE(bad_status.erase_payload(kUrl1));
  EXPECT_TRUE(bad_status.erase_payload(kUrl3));

  bad_status.set_payload(kUrl1, turbo::Cord(kPayload1));
  EXPECT_TRUE(bad_status.erase_payload(kUrl1));
}

TEST(Status, TestComparePayloads) {
  turbo::Status bad_status1(turbo::StatusCode::kInternal, "fail");
  bad_status1.set_payload(kUrl1, turbo::Cord(kPayload1));
  bad_status1.set_payload(kUrl2, turbo::Cord(kPayload2));
  bad_status1.set_payload(kUrl3, turbo::Cord(kPayload3));

  turbo::Status bad_status2(turbo::StatusCode::kInternal, "fail");
  bad_status2.set_payload(kUrl2, turbo::Cord(kPayload2));
  bad_status2.set_payload(kUrl3, turbo::Cord(kPayload3));
  bad_status2.set_payload(kUrl1, turbo::Cord(kPayload1));

  EXPECT_EQ(bad_status1, bad_status2);
}

TEST(Status, TestComparePayloadsAfterErase) {
  turbo::Status payload_status(turbo::StatusCode::kInternal, "");
  payload_status.set_payload(kUrl1, turbo::Cord(kPayload1));
  payload_status.set_payload(kUrl2, turbo::Cord(kPayload2));

  turbo::Status empty_status(turbo::StatusCode::kInternal, "");

  // Different payloads, not equal
  EXPECT_NE(payload_status, empty_status);
  EXPECT_TRUE(payload_status.erase_payload(kUrl1));

  // Still Different payloads, still not equal.
  EXPECT_NE(payload_status, empty_status);
  EXPECT_TRUE(payload_status.erase_payload(kUrl2));

  // Both empty payloads, should be equal
  EXPECT_EQ(payload_status, empty_status);
}

PayloadsVec AllVisitedPayloads(const turbo::Status& s) {
  PayloadsVec result;

  s.for_each_payload([&](std::string_view type_url, const turbo::Cord& payload) {
    result.push_back(std::make_pair(std::string(type_url), payload));
  });

  return result;
}

TEST(Status, TestForEachPayload) {
  turbo::Status bad_status(turbo::StatusCode::kInternal, "fail");
  bad_status.set_payload(kUrl1, turbo::Cord(kPayload1));
  bad_status.set_payload(kUrl2, turbo::Cord(kPayload2));
  bad_status.set_payload(kUrl3, turbo::Cord(kPayload3));

  int count = 0;

  bad_status.for_each_payload(
      [&count](std::string_view, const turbo::Cord&) { ++count; });

  EXPECT_EQ(count, 3);

  PayloadsVec expected_payloads = {{kUrl1, turbo::Cord(kPayload1)},
                                   {kUrl2, turbo::Cord(kPayload2)},
                                   {kUrl3, turbo::Cord(kPayload3)}};

  // Test that we visit all the payloads in the status.
  PayloadsVec visited_payloads = AllVisitedPayloads(bad_status);
  EXPECT_THAT(visited_payloads, UnorderedElementsAreArray(expected_payloads));

  // Test that visitation order is not consistent between run.
  std::vector<turbo::Status> scratch;
  while (true) {
    scratch.emplace_back(turbo::StatusCode::kInternal, "fail");

    scratch.back().set_payload(kUrl1, turbo::Cord(kPayload1));
    scratch.back().set_payload(kUrl2, turbo::Cord(kPayload2));
    scratch.back().set_payload(kUrl3, turbo::Cord(kPayload3));

    if (AllVisitedPayloads(scratch.back()) != visited_payloads) {
      break;
    }
  }
}

TEST(Status, ToString) {
  turbo::Status status(turbo::StatusCode::kInternal, "fail");
  EXPECT_EQ("INTERNAL: fail", status.to_string());
  status.set_payload("foo", turbo::Cord("bar"));
  EXPECT_EQ("INTERNAL: fail [foo='bar']", status.to_string());
  status.set_payload("bar", turbo::Cord("\377"));
  EXPECT_THAT(status.to_string(),
              AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                    HasSubstr("[bar='\\xff']")));
}

TEST(Status, ToStringMode) {
  turbo::Status status(turbo::StatusCode::kInternal, "fail");
  status.set_payload("foo", turbo::Cord("bar"));
  status.set_payload("bar", turbo::Cord("\377"));

  EXPECT_EQ("INTERNAL: fail",
            status.to_string(turbo::StatusToStringMode::kWithNoExtraData));

  EXPECT_THAT(status.to_string(turbo::StatusToStringMode::kWithPayload),
              AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                    HasSubstr("[bar='\\xff']")));

  EXPECT_THAT(status.to_string(turbo::StatusToStringMode::kWithEverything),
              AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                    HasSubstr("[bar='\\xff']")));

  EXPECT_THAT(status.to_string(~turbo::StatusToStringMode::kWithPayload),
              AllOf(HasSubstr("INTERNAL: fail"), Not(HasSubstr("[foo='bar']")),
                    Not(HasSubstr("[bar='\\xff']"))));
}

TEST(Status, OstreamOperator) {
  turbo::Status status(turbo::StatusCode::kInternal, "fail");
  { std::stringstream stream;
    stream << status;
    EXPECT_EQ("INTERNAL: fail", stream.str());
  }
  status.set_payload("foo", turbo::Cord("bar"));
  { std::stringstream stream;
    stream << status;
    EXPECT_EQ("INTERNAL: fail [foo='bar']", stream.str());
  }
  status.set_payload("bar", turbo::Cord("\377"));
  { std::stringstream stream;
    stream << status;
    EXPECT_THAT(stream.str(),
                AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                      HasSubstr("[bar='\\xff']")));
  }
}

TEST(Status, turbo_stringify) {
  turbo::Status status(turbo::StatusCode::kInternal, "fail");
  EXPECT_EQ("INTERNAL: fail", turbo::str_cat(status));
  EXPECT_EQ("INTERNAL: fail", turbo::str_format("%v", status));
  status.set_payload("foo", turbo::Cord("bar"));
  EXPECT_EQ("INTERNAL: fail [foo='bar']", turbo::str_cat(status));
  status.set_payload("bar", turbo::Cord("\377"));
  EXPECT_THAT(turbo::str_cat(status),
              AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                    HasSubstr("[bar='\\xff']")));
}

TEST(Status, OstreamEqStringify) {
  turbo::Status status(turbo::StatusCode::kUnknown, "fail");
  status.set_payload("foo", turbo::Cord("bar"));
  std::stringstream stream;
  stream << status;
  EXPECT_EQ(stream.str(), turbo::str_cat(status));
}

turbo::Status EraseAndReturn(const turbo::Status& base) {
  turbo::Status copy = base;
  EXPECT_TRUE(copy.erase_payload(kUrl1));
  return copy;
}

TEST(Status, CopyOnWriteForErasePayload) {
  {
    turbo::Status base(turbo::StatusCode::kInvalidArgument, "fail");
    base.set_payload(kUrl1, turbo::Cord(kPayload1));
    EXPECT_TRUE(base.get_payload(kUrl1).has_value());
    turbo::Status copy = EraseAndReturn(base);
    EXPECT_TRUE(base.get_payload(kUrl1).has_value());
    EXPECT_FALSE(copy.get_payload(kUrl1).has_value());
  }
  {
    turbo::Status base(turbo::StatusCode::kInvalidArgument, "fail");
    base.set_payload(kUrl1, turbo::Cord(kPayload1));
    turbo::Status copy = base;

    EXPECT_TRUE(base.get_payload(kUrl1).has_value());
    EXPECT_TRUE(copy.get_payload(kUrl1).has_value());

    EXPECT_TRUE(base.erase_payload(kUrl1));

    EXPECT_FALSE(base.get_payload(kUrl1).has_value());
    EXPECT_TRUE(copy.get_payload(kUrl1).has_value());
  }
}

TEST(Status, CopyConstructor) {
  {
    turbo::Status status;
    turbo::Status copy(status);
    EXPECT_EQ(copy, status);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    turbo::Status copy(status);
    EXPECT_EQ(copy, status);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    status.set_payload(kUrl1, turbo::Cord(kPayload1));
    turbo::Status copy(status);
    EXPECT_EQ(copy, status);
  }
}

TEST(Status, CopyAssignment) {
  turbo::Status assignee;
  {
    turbo::Status status;
    assignee = status;
    EXPECT_EQ(assignee, status);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    assignee = status;
    EXPECT_EQ(assignee, status);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    status.set_payload(kUrl1, turbo::Cord(kPayload1));
    assignee = status;
    EXPECT_EQ(assignee, status);
  }
}

TEST(Status, CopyAssignmentIsNotRef) {
  const turbo::Status status_orig(turbo::StatusCode::kInvalidArgument, "message");
  turbo::Status status_copy = status_orig;
  EXPECT_EQ(status_orig, status_copy);
  status_copy.set_payload(kUrl1, turbo::Cord(kPayload1));
  EXPECT_NE(status_orig, status_copy);
}

TEST(Status, MoveConstructor) {
  {
    turbo::Status status;
    turbo::Status copy(turbo::Status{});
    EXPECT_EQ(copy, status);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    turbo::Status copy(
        turbo::Status(turbo::StatusCode::kInvalidArgument, "message"));
    EXPECT_EQ(copy, status);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    status.set_payload(kUrl1, turbo::Cord(kPayload1));
    turbo::Status copy1(status);
    turbo::Status copy2(std::move(status));
    EXPECT_EQ(copy1, copy2);
  }
}

TEST(Status, MoveAssignment) {
  turbo::Status assignee;
  {
    turbo::Status status;
    assignee = turbo::Status();
    EXPECT_EQ(assignee, status);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    assignee = turbo::Status(turbo::StatusCode::kInvalidArgument, "message");
    EXPECT_EQ(assignee, status);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    status.set_payload(kUrl1, turbo::Cord(kPayload1));
    turbo::Status copy(status);
    assignee = std::move(status);
    EXPECT_EQ(assignee, copy);
  }
  {
    turbo::Status status(turbo::StatusCode::kInvalidArgument, "message");
    turbo::Status copy(status);
    assignee = static_cast<turbo::Status&&>(status);
    EXPECT_EQ(assignee, copy);
  }
}

TEST(Status, Update) {
  turbo::Status s;
  s.update(turbo::OkStatus());
  EXPECT_TRUE(s.ok());
  const turbo::Status a(turbo::StatusCode::kCancelled, "message");
  s.update(a);
  EXPECT_EQ(s, a);
  const turbo::Status b(turbo::StatusCode::kInternal, "other message");
  s.update(b);
  EXPECT_EQ(s, a);
  s.update(turbo::OkStatus());
  EXPECT_EQ(s, a);
  EXPECT_FALSE(s.ok());
}

TEST(Status, Equality) {
  turbo::Status ok;
  turbo::Status no_payload = turbo::cancelled_error("no payload");
  turbo::Status one_payload = turbo::invalid_argument_error("one payload");
  one_payload.set_payload(kUrl1, turbo::Cord(kPayload1));
  turbo::Status two_payloads = one_payload;
  two_payloads.set_payload(kUrl2, turbo::Cord(kPayload2));
  const std::array<turbo::Status, 4> status_arr = {ok, no_payload, one_payload,
                                                  two_payloads};
  for (int i = 0; i < status_arr.size(); i++) {
    for (int j = 0; j < status_arr.size(); j++) {
      if (i == j) {
        EXPECT_TRUE(status_arr[i] == status_arr[j]);
        EXPECT_FALSE(status_arr[i] != status_arr[j]);
      } else {
        EXPECT_TRUE(status_arr[i] != status_arr[j]);
        EXPECT_FALSE(status_arr[i] == status_arr[j]);
      }
    }
  }
}

TEST(Status, Swap) {
  auto test_swap = [](const turbo::Status& s1, const turbo::Status& s2) {
    turbo::Status copy1 = s1, copy2 = s2;
    swap(copy1, copy2);
    EXPECT_EQ(copy1, s2);
    EXPECT_EQ(copy2, s1);
  };
  const turbo::Status ok;
  const turbo::Status no_payload(turbo::StatusCode::kAlreadyExists, "no payload");
  turbo::Status with_payload(turbo::StatusCode::kInternal, "with payload");
  with_payload.set_payload(kUrl1, turbo::Cord(kPayload1));
  test_swap(ok, no_payload);
  test_swap(no_payload, ok);
  test_swap(ok, with_payload);
  test_swap(with_payload, ok);
  test_swap(no_payload, with_payload);
  test_swap(with_payload, no_payload);
}

TEST(StatusErrno, errno_to_status_code) {
  EXPECT_EQ(turbo::errno_to_status_code(0), turbo::StatusCode::kOk);

  // Spot-check a few errno values.
  EXPECT_EQ(turbo::errno_to_status_code(EINVAL),
            turbo::StatusCode::kInvalidArgument);
  EXPECT_EQ(turbo::errno_to_status_code(ENOENT), turbo::StatusCode::kNotFound);

  // We'll pick a very large number so it hopefully doesn't collide to errno.
  EXPECT_EQ(turbo::errno_to_status_code(19980927), turbo::StatusCode::kUnknown);
}

TEST(StatusErrno, errno_to_status) {
  turbo::Status status = turbo::errno_to_status(ENOENT, "Cannot open 'path'");
  EXPECT_EQ(status.code(), turbo::StatusCode::kNotFound);
  EXPECT_EQ(status.message(), "Cannot open 'path': No such file or directory");
}

}  // namespace
