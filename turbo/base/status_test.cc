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

#include "turbo/base/status.h"

#include <errno.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/strings/str_cat.h"
#include "turbo/base/turbo_error.h"
#include "turbo/base/turbo_module.h"

TURBO_REGISTER_ERRNO(30, "TEST_ERROR");
TURBO_REGISTER_MODULE_INDEX(1, "TEST_MODULE");

namespace {

using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Optional;
using ::testing::UnorderedElementsAreArray;

TEST(StatusCode, registry) {
        const turbo::StatusCode code = 30;
        EXPECT_EQ(turbo::StatusCodeToString(code), "TEST_ERROR");
        turbo::Status s(30, "");
        EXPECT_EQ(s.code(), 30);
        EXPECT_EQ(s.raw_code(), 30);
        EXPECT_EQ(s.index(), 0);

        turbo::Status si(1, 30, "");
        EXPECT_EQ(si.raw_code(), 30);
        EXPECT_EQ(si.index(), 1);
        std::ostringstream oss;

        oss<<si;
        EXPECT_EQ(oss.str(), "TEST_MODULE::TEST_ERROR: ");

        turbo::Status sim(1, 30, "fail");
        std::ostringstream oss1;

        oss1<<sim;
        EXPECT_EQ(oss1.str(), "TEST_MODULE::TEST_ERROR: fail");
}
TEST(StatusCode, InsertionOperator) {
  const turbo::StatusCode code = turbo::kUnknown;
  std::ostringstream oss;
  oss << code;
  EXPECT_EQ(oss.str(), "2");
  EXPECT_EQ(turbo::StatusCodeToString(code), "UNKNOWN");
}

// This structure holds the details for testing a single error code,
// its creator, and its classifier.
struct ErrorTest {
  turbo::StatusCode code;
  using Creator = turbo::Status (*)(
      turbo::string_view
  );
  using Classifier = bool (*)(const turbo::Status&);
  Creator creator;
  Classifier classifier;
};

constexpr ErrorTest kErrorTests[]{
    {turbo::kCancelled, turbo::CancelledError, turbo::IsCancelled},
    {turbo::kUnknown, turbo::UnknownError, turbo::IsUnknown},
    {turbo::kInvalidArgument, turbo::InvalidArgumentError,
     turbo::IsInvalidArgument},
    {turbo::kDeadlineExceeded, turbo::DeadlineExceededError,
     turbo::IsDeadlineExceeded},
    {turbo::kNotFound, turbo::NotFoundError, turbo::IsNotFound},
    {turbo::kAlreadyExists, turbo::AlreadyExistsError,
     turbo::IsAlreadyExists},
    {turbo::kPermissionDenied, turbo::PermissionDeniedError,
     turbo::IsPermissionDenied},
    {turbo::kResourceExhausted, turbo::ResourceExhaustedError,
     turbo::IsResourceExhausted},
    {turbo::kFailedPrecondition, turbo::FailedPreconditionError,
     turbo::IsFailedPrecondition},
    {turbo::kAborted, turbo::AbortedError, turbo::IsAborted},
    {turbo::kOutOfRange, turbo::OutOfRangeError, turbo::IsOutOfRange},
    {turbo::kUnimplemented, turbo::UnimplementedError,
     turbo::IsUnimplemented},
    {turbo::kInternal, turbo::InternalError, turbo::IsInternal},
    {turbo::kUnavailable, turbo::UnavailableError,
     turbo::IsUnavailable},
    {turbo::kDataLoss, turbo::DataLossError, turbo::IsDataLoss},
    {turbo::kUnauthenticated, turbo::UnauthenticatedError,
     turbo::IsUnauthenticated},
};

TEST(Status, CreateAndClassify) {
  for (const auto& test : kErrorTests) {
    SCOPED_TRACE(turbo::StatusCodeToString(test.code));

    // Ensure that the creator does, in fact, create status objects with the
    // expected error code and message.
    std::string message =
        turbo::StrCat("error code ", test.code, " test message");
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
  EXPECT_EQ(turbo::kOk, status.code());
  EXPECT_EQ("", status.message());
}

TEST(Status, OkStatus) {
  turbo::Status status = turbo::OkStatus();
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(turbo::kOk, status.code());
  EXPECT_EQ("", status.message());
}

TEST(Status, ConstructorWithCodeMessage) {
  {
    turbo::Status status(turbo::kCancelled, "");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(turbo::kCancelled, status.code());
    EXPECT_EQ("", status.message());
  }
  {
    turbo::Status status(turbo::kInternal, "message");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(turbo::kInternal, status.code());
    EXPECT_EQ("message", status.message());
  }
}

TEST(Status, ConstructOutOfRangeCode) {
  const int kRawCode = 9999;
  turbo::Status status(static_cast<turbo::StatusCode>(kRawCode), "");
  //EXPECT_EQ(turbo::kUnknown, status.code());
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
  ok_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
  ok_status.SetPayload(kUrl2, turbo::Cord(kPayload2));

  EXPECT_FALSE(ok_status.GetPayload(kUrl1));
  EXPECT_FALSE(ok_status.GetPayload(kUrl2));

  turbo::Status bad_status(turbo::kInternal, "fail");
  bad_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
  bad_status.SetPayload(kUrl2, turbo::Cord(kPayload2));

  EXPECT_THAT(bad_status.GetPayload(kUrl1), Optional(Eq(kPayload1)));
  EXPECT_THAT(bad_status.GetPayload(kUrl2), Optional(Eq(kPayload2)));

  EXPECT_FALSE(bad_status.GetPayload(kUrl3));

  bad_status.SetPayload(kUrl1, turbo::Cord(kPayload3));
  EXPECT_THAT(bad_status.GetPayload(kUrl1), Optional(Eq(kPayload3)));

  // Testing dynamically generated type_url
  bad_status.SetPayload(turbo::StrCat(kUrl1, ".1"), turbo::Cord(kPayload1));
  EXPECT_THAT(bad_status.GetPayload(turbo::StrCat(kUrl1, ".1")),
              Optional(Eq(kPayload1)));
}

TEST(Status, TestErasePayload) {
  turbo::Status bad_status(turbo::kInternal, "fail");
  bad_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
  bad_status.SetPayload(kUrl2, turbo::Cord(kPayload2));
  bad_status.SetPayload(kUrl3, turbo::Cord(kPayload3));

  EXPECT_FALSE(bad_status.ErasePayload(kUrl4));

  EXPECT_TRUE(bad_status.GetPayload(kUrl2));
  EXPECT_TRUE(bad_status.ErasePayload(kUrl2));
  EXPECT_FALSE(bad_status.GetPayload(kUrl2));
  EXPECT_FALSE(bad_status.ErasePayload(kUrl2));

  EXPECT_TRUE(bad_status.ErasePayload(kUrl1));
  EXPECT_TRUE(bad_status.ErasePayload(kUrl3));

  bad_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
  EXPECT_TRUE(bad_status.ErasePayload(kUrl1));
}

TEST(Status, TestComparePayloads) {
  turbo::Status bad_status1(turbo::kInternal, "fail");
  bad_status1.SetPayload(kUrl1, turbo::Cord(kPayload1));
  bad_status1.SetPayload(kUrl2, turbo::Cord(kPayload2));
  bad_status1.SetPayload(kUrl3, turbo::Cord(kPayload3));

  turbo::Status bad_status2(turbo::kInternal, "fail");
  bad_status2.SetPayload(kUrl2, turbo::Cord(kPayload2));
  bad_status2.SetPayload(kUrl3, turbo::Cord(kPayload3));
  bad_status2.SetPayload(kUrl1, turbo::Cord(kPayload1));

  EXPECT_EQ(bad_status1, bad_status2);
}

TEST(Status, TestComparePayloadsAfterErase) {
  turbo::Status payload_status(turbo::kInternal, "");
  payload_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
  payload_status.SetPayload(kUrl2, turbo::Cord(kPayload2));

  turbo::Status empty_status(turbo::kInternal, "");

  // Different payloads, not equal
  EXPECT_NE(payload_status, empty_status);
  EXPECT_TRUE(payload_status.ErasePayload(kUrl1));

  // Still Different payloads, still not equal.
  EXPECT_NE(payload_status, empty_status);
  EXPECT_TRUE(payload_status.ErasePayload(kUrl2));

  // Both empty payloads, should be equal
  EXPECT_EQ(payload_status, empty_status);
}

PayloadsVec AllVisitedPayloads(const turbo::Status& s) {
  PayloadsVec result;

  s.ForEachPayload([&](turbo::string_view type_url, const turbo::Cord& payload) {
    result.push_back(std::make_pair(std::string(type_url), payload));
  });

  return result;
}

TEST(Status, TestForEachPayload) {
  turbo::Status bad_status(turbo::kInternal, "fail");
  bad_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
  bad_status.SetPayload(kUrl2, turbo::Cord(kPayload2));
  bad_status.SetPayload(kUrl3, turbo::Cord(kPayload3));

  int count = 0;

  bad_status.ForEachPayload(
      [&count](turbo::string_view, const turbo::Cord&) { ++count; });

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
    scratch.emplace_back(turbo::kInternal, "fail");

    scratch.back().SetPayload(kUrl1, turbo::Cord(kPayload1));
    scratch.back().SetPayload(kUrl2, turbo::Cord(kPayload2));
    scratch.back().SetPayload(kUrl3, turbo::Cord(kPayload3));

    if (AllVisitedPayloads(scratch.back()) != visited_payloads) {
      break;
    }
  }
}

TEST(Status, ToString) {
  turbo::Status s(turbo::kInternal, "fail");
  EXPECT_EQ("INTERNAL: fail", s.ToString());
  s.SetPayload("foo", turbo::Cord("bar"));
  EXPECT_EQ("INTERNAL: fail [foo='bar']", s.ToString());
  s.SetPayload("bar", turbo::Cord("\377"));
  EXPECT_THAT(s.ToString(),
              AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                    HasSubstr("[bar='\\xff']")));
}

TEST(Status, ToStringMode) {
  turbo::Status s(turbo::kInternal, "fail");
  s.SetPayload("foo", turbo::Cord("bar"));
  s.SetPayload("bar", turbo::Cord("\377"));

  EXPECT_EQ("INTERNAL: fail",
            s.ToString(turbo::StatusToStringMode::kWithNoExtraData));

  EXPECT_THAT(s.ToString(turbo::StatusToStringMode::kWithPayload),
              AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                    HasSubstr("[bar='\\xff']")));

  EXPECT_THAT(s.ToString(turbo::StatusToStringMode::kWithEverything),
              AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                    HasSubstr("[bar='\\xff']")));

  EXPECT_THAT(s.ToString(~turbo::StatusToStringMode::kWithPayload),
              AllOf(HasSubstr("INTERNAL: fail"), Not(HasSubstr("[foo='bar']")),
                    Not(HasSubstr("[bar='\\xff']"))));
}

turbo::Status EraseAndReturn(const turbo::Status& base) {
  turbo::Status copy = base;
  EXPECT_TRUE(copy.ErasePayload(kUrl1));
  return copy;
}

TEST(Status, CopyOnWriteForErasePayload) {
  {
    turbo::Status base(turbo::kInvalidArgument, "fail");
    base.SetPayload(kUrl1, turbo::Cord(kPayload1));
    EXPECT_TRUE(base.GetPayload(kUrl1).has_value());
    turbo::Status copy = EraseAndReturn(base);
    EXPECT_TRUE(base.GetPayload(kUrl1).has_value());
    EXPECT_FALSE(copy.GetPayload(kUrl1).has_value());
  }
  {
    turbo::Status base(turbo::kInvalidArgument, "fail");
    base.SetPayload(kUrl1, turbo::Cord(kPayload1));
    turbo::Status copy = base;

    EXPECT_TRUE(base.GetPayload(kUrl1).has_value());
    EXPECT_TRUE(copy.GetPayload(kUrl1).has_value());

    EXPECT_TRUE(base.ErasePayload(kUrl1));

    EXPECT_FALSE(base.GetPayload(kUrl1).has_value());
    EXPECT_TRUE(copy.GetPayload(kUrl1).has_value());
  }
}

TEST(Status, CopyConstructor) {
  {
    turbo::Status status;
    turbo::Status copy(status);
    EXPECT_EQ(copy, status);
  }
  {
    turbo::Status status(turbo::kInvalidArgument, "message");
    turbo::Status copy(status);
    EXPECT_EQ(copy, status);
  }
  {
    turbo::Status status(turbo::kInvalidArgument, "message");
    status.SetPayload(kUrl1, turbo::Cord(kPayload1));
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
    turbo::Status status(turbo::kInvalidArgument, "message");
    assignee = status;
    EXPECT_EQ(assignee, status);
  }
  {
    turbo::Status status(turbo::kInvalidArgument, "message");
    status.SetPayload(kUrl1, turbo::Cord(kPayload1));
    assignee = status;
    EXPECT_EQ(assignee, status);
  }
}

TEST(Status, CopyAssignmentIsNotRef) {
  const turbo::Status status_orig(turbo::kInvalidArgument, "message");
  turbo::Status status_copy = status_orig;
  EXPECT_EQ(status_orig, status_copy);
  status_copy.SetPayload(kUrl1, turbo::Cord(kPayload1));
  EXPECT_NE(status_orig, status_copy);
}

TEST(Status, MoveConstructor) {
  {
    turbo::Status status;
    turbo::Status copy(turbo::Status{});
    EXPECT_EQ(copy, status);
  }
  {
    turbo::Status status(turbo::kInvalidArgument, "message");
    turbo::Status copy(
        turbo::Status(turbo::kInvalidArgument, "message"));
    EXPECT_EQ(copy, status);
  }
  {
    turbo::Status status(turbo::kInvalidArgument, "message");
    status.SetPayload(kUrl1, turbo::Cord(kPayload1));
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
    turbo::Status status(turbo::kInvalidArgument, "message");
    assignee = turbo::Status(turbo::kInvalidArgument, "message");
    EXPECT_EQ(assignee, status);
  }
  {
    turbo::Status status(turbo::kInvalidArgument, "message");
    status.SetPayload(kUrl1, turbo::Cord(kPayload1));
    turbo::Status copy(status);
    assignee = std::move(status);
    EXPECT_EQ(assignee, copy);
  }
  {
    turbo::Status status(turbo::kInvalidArgument, "message");
    turbo::Status copy(status);
    status = static_cast<turbo::Status&&>(status);
    EXPECT_EQ(status, copy);
  }
}

TEST(Status, Update) {
  turbo::Status s;
  s.Update(turbo::OkStatus());
  EXPECT_TRUE(s.ok());
  const turbo::Status a(turbo::kCancelled, "message");
  s.Update(a);
  EXPECT_EQ(s, a);
  const turbo::Status b(turbo::kInternal, "other message");
  s.Update(b);
  EXPECT_EQ(s, a);
  s.Update(turbo::OkStatus());
  EXPECT_EQ(s, a);
  EXPECT_FALSE(s.ok());
}

TEST(Status, Equality) {
  turbo::Status ok;
  turbo::Status no_payload = turbo::CancelledError("no payload");
  turbo::Status one_payload = turbo::InvalidArgumentError("one payload");
  one_payload.SetPayload(kUrl1, turbo::Cord(kPayload1));
  turbo::Status two_payloads = one_payload;
  two_payloads.SetPayload(kUrl2, turbo::Cord(kPayload2));
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
  const turbo::Status no_payload(turbo::kAlreadyExists, "no payload");
  turbo::Status with_payload(turbo::kInternal, "with payload");
  with_payload.SetPayload(kUrl1, turbo::Cord(kPayload1));
  test_swap(ok, no_payload);
  test_swap(no_payload, ok);
  test_swap(ok, with_payload);
  test_swap(with_payload, ok);
  test_swap(no_payload, with_payload);
  test_swap(with_payload, no_payload);
}

TEST(StatusErrno, ErrnoToStatusCode) {
  EXPECT_EQ(turbo::ErrnoToStatusCode(0), turbo::kOk);

  // Spot-check a few errno values.
  EXPECT_EQ(turbo::ErrnoToStatusCode(EINVAL),
            turbo::kInvalidArgument);
  EXPECT_EQ(turbo::ErrnoToStatusCode(ENOENT), turbo::kNotFound);

  // We'll pick a very large number so it hopefully doesn't collide to errno.
  EXPECT_EQ(turbo::ErrnoToStatusCode(19980927), turbo::kUnknown);
}

TEST(StatusErrno, ErrnoToStatus) {
  turbo::Status status = turbo::ErrnoToStatus(ENOENT, "Cannot open 'path'");
  EXPECT_EQ(status.code(), turbo::kNotFound);
  EXPECT_EQ(status.message(), "Cannot open 'path': No such file or directory");
}

}  // namespace
