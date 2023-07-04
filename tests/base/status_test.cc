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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "tests/doctest/doctest.h"

#include "turbo/format/format.h"
#include "turbo/base/turbo_error.h"
#include "turbo/base/turbo_module.h"

TURBO_REGISTER_ERRNO(30, "TEST_ERROR");
TURBO_REGISTER_MODULE_INDEX(1, "TEST_MODULE");

namespace {
    TEST_CASE("StatusCode") {
        SUBCASE("registry")
        {
            const turbo::StatusCode code = 30;
            REQUIRE_EQ(turbo::StatusCodeToString(code), "TEST_ERROR");
            turbo::Status s(30, "");
            REQUIRE_EQ(s.code(), 30);
            REQUIRE_EQ(s.raw_code(), 30);
            REQUIRE_EQ(s.index(), 0);

            turbo::Status si(1, 30, "");
            REQUIRE_EQ(si.raw_code(), 30);
            REQUIRE_EQ(si.index(), 1);
            std::ostringstream oss;

            oss << si;
            REQUIRE_EQ(oss.str(), "TEST_MODULE::TEST_ERROR: ");

            turbo::Status sim(1, 30, "fail");
            std::ostringstream oss1;

            oss1 << sim;
            REQUIRE_EQ(oss1.str(), "TEST_MODULE::TEST_ERROR: fail");
        }

        SUBCASE("InsertionOperator")
        {
            const turbo::StatusCode code = turbo::kUnknown;
            std::ostringstream oss;
            oss << code;
            REQUIRE_EQ(oss.str(), "2");
            REQUIRE_EQ(turbo::StatusCodeToString(code), "UNKNOWN");
        }
    }
    // This structure holds the details for testing a single error code,
    // its creator, and its classifier.
    struct ErrorTest {
        turbo::StatusCode code;
        using Creator = turbo::Status (*)(
                std::string_view
        );
        using Classifier = bool (*)(const turbo::Status &);
        Creator creator;
        Classifier classifier;
    };

    constexpr ErrorTest kErrorTests[]{
            {turbo::kCancelled,          turbo::CancelledError,  turbo::IsCancelled},
            {turbo::kUnknown,            turbo::UnknownError,    turbo::IsUnknown},
            {turbo::kInvalidArgument,    turbo::InvalidArgumentError,
                                                                 turbo::IsInvalidArgument},
            {turbo::kDeadlineExceeded,   turbo::DeadlineExceededError,
                                                                 turbo::IsDeadlineExceeded},
            {turbo::kNotFound,           turbo::NotFoundError,   turbo::IsNotFound},
            {turbo::kAlreadyExists,      turbo::AlreadyExistsError,
                                                                 turbo::IsAlreadyExists},
            {turbo::kPermissionDenied,   turbo::PermissionDeniedError,
                                                                 turbo::IsPermissionDenied},
            {turbo::kResourceExhausted,  turbo::ResourceExhaustedError,
                                                                 turbo::IsResourceExhausted},
            {turbo::kFailedPrecondition, turbo::FailedPreconditionError,
                                                                 turbo::IsFailedPrecondition},
            {turbo::kAborted,            turbo::AbortedError,    turbo::IsAborted},
            {turbo::kOutOfRange,         turbo::OutOfRangeError, turbo::IsOutOfRange},
            {turbo::kUnimplemented,      turbo::UnimplementedError,
                                                                 turbo::IsUnimplemented},
            {turbo::kInternal,           turbo::InternalError,   turbo::IsInternal},
            {turbo::kUnavailable,        turbo::UnavailableError,
                                                                 turbo::IsUnavailable},
            {turbo::kDataLoss,           turbo::DataLossError,   turbo::IsDataLoss},
            {turbo::kUnauthenticated,    turbo::UnauthenticatedError,
                                                                 turbo::IsUnauthenticated},
    };

    constexpr char kUrl1[] = "url.payload.1";
    constexpr char kUrl2[] = "url.payload.2";
    constexpr char kUrl3[] = "url.payload.3";
    constexpr char kUrl4[] = "url.payload.xx";

    constexpr char kPayload1[] = "aaaaa";
    constexpr char kPayload2[] = "bbbbb";
    constexpr char kPayload3[] = "ccccc";
    turbo::Status EraseAndReturn(const turbo::Status &base) {
        turbo::Status copy = base;
        REQUIRE(copy.ErasePayload(kUrl1));
        return copy;
    }
    using PayloadsVec = std::vector<std::pair<std::string, turbo::Cord>>;
    PayloadsVec AllVisitedPayloads(const turbo::Status &s) {
        PayloadsVec result;

        s.ForEachPayload([&](std::string_view type_url, const turbo::Cord &payload) {
            result.push_back(std::make_pair(std::string(type_url), payload));
        });

        return result;
    }

    TEST_CASE("Status") {
        SUBCASE("CreateAndClassify") {
            for (const auto &test: kErrorTests) {
                CAPTURE(turbo::StatusCodeToString(test.code));

                // Ensure that the creator does, in fact, create status objects with the
                // expected error code and message.
                std::string message =
                        turbo::Format("error code {} test message", test.code);
                turbo::Status status = test.creator(
                        message
                );
                REQUIRE_EQ(test.code, status.code());
                REQUIRE_EQ(message, status.message());

                // Ensure that the classifier returns true for a status produced by the
                // creator.
                REQUIRE(test.classifier(status));

                // Ensure that the classifier returns false for status with a different
                // code.
                for (const auto &other: kErrorTests) {
                    if (other.code != test.code) {
                        REQUIRE_FALSE(test.classifier(turbo::Status(other.code, "")));
                        //<< " other.code = " << other.code;
                    }
                }
            }
        }

        SUBCASE(" DefaultConstructor") {
            turbo::Status status;
            REQUIRE(status.ok());
            REQUIRE_EQ(turbo::kOk, status.code());
            REQUIRE_EQ("", status.message());
        }

        SUBCASE(" OkStatus") {
            turbo::Status status = turbo::OkStatus();
            REQUIRE(status.ok());
            REQUIRE_EQ(turbo::kOk, status.code());
            REQUIRE_EQ("", status.message());
        }

        SUBCASE(" MakeStatus") {
            turbo::Status status = turbo::MakeStatus(100, "this is {} error", 100);
            REQUIRE_FALSE(status.ok());
            REQUIRE_EQ(100, status.code());
            REQUIRE_EQ("this is 100 error", status.message());
        }

        SUBCASE(" ConstructorWithCodeMessage") {
            {
                turbo::Status status(turbo::kCancelled, "");
                REQUIRE_FALSE(status.ok());
                REQUIRE_EQ(turbo::kCancelled, status.code());
                REQUIRE_EQ("", status.message());
            }
            {
                turbo::Status status(turbo::kInternal, "message");
                REQUIRE_FALSE(status.ok());
                REQUIRE_EQ(turbo::kInternal, status.code());
                REQUIRE_EQ("message", status.message());
            }
        }

        SUBCASE(" ConstructOutOfRangeCode") {
            const int kRawCode = 9999;
            turbo::Status status(static_cast<turbo::StatusCode>(kRawCode), "");
            //REQUIRE_EQ(turbo::kUnknown, status.code());
            REQUIRE_EQ(kRawCode, status.raw_code());
        }





        /*
        SUBCASE(" TestGetSetPayload") {
            turbo::Status ok_status = turbo::OkStatus();
            ok_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
            ok_status.SetPayload(kUrl2, turbo::Cord(kPayload2));

            REQUIRE_FALSE(ok_status.GetPayload(kUrl1));
            REQUIRE_FALSE(ok_status.GetPayload(kUrl2));

            turbo::Status bad_status(turbo::kInternal, "fail");
            bad_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
            bad_status.SetPayload(kUrl2, turbo::Cord(kPayload2));

            EXPECT_THAT(bad_status.GetPayload(kUrl1), Optional(Eq(kPayload1)));
            EXPECT_THAT(bad_status.GetPayload(kUrl2), Optional(Eq(kPayload2)));

            REQUIRE_FALSE(bad_status.GetPayload(kUrl3));

            bad_status.SetPayload(kUrl1, turbo::Cord(kPayload3));
            EXPECT_THAT(bad_status.GetPayload(kUrl1), Optional(Eq(kPayload3)));

            // Testing dynamically generated type_url
            bad_status.SetPayload(turbo::Format("{}{}", kUrl1, ".1"), turbo::Cord(kPayload1));
            EXPECT_THAT(bad_status.GetPayload(turbo::Format("{}{}", kUrl1, ".1")),
                        Optional(Eq(kPayload1)));
        }
        */
        SUBCASE(" TestErasePayload") {
            turbo::Status bad_status(turbo::kInternal, "fail");
            bad_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
            bad_status.SetPayload(kUrl2, turbo::Cord(kPayload2));
            bad_status.SetPayload(kUrl3, turbo::Cord(kPayload3));

            REQUIRE_FALSE(bad_status.ErasePayload(kUrl4));

            REQUIRE(bad_status.GetPayload(kUrl2));
            REQUIRE(bad_status.ErasePayload(kUrl2));
            REQUIRE_FALSE(bad_status.GetPayload(kUrl2));
            REQUIRE_FALSE(bad_status.ErasePayload(kUrl2));

            REQUIRE(bad_status.ErasePayload(kUrl1));
            REQUIRE(bad_status.ErasePayload(kUrl3));

            bad_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
            REQUIRE(bad_status.ErasePayload(kUrl1));
        }

        SUBCASE(" TestComparePayloads") {
            turbo::Status bad_status1(turbo::kInternal, "fail");
            bad_status1.SetPayload(kUrl1, turbo::Cord(kPayload1));
            bad_status1.SetPayload(kUrl2, turbo::Cord(kPayload2));
            bad_status1.SetPayload(kUrl3, turbo::Cord(kPayload3));

            turbo::Status bad_status2(turbo::kInternal, "fail");
            bad_status2.SetPayload(kUrl2, turbo::Cord(kPayload2));
            bad_status2.SetPayload(kUrl3, turbo::Cord(kPayload3));
            bad_status2.SetPayload(kUrl1, turbo::Cord(kPayload1));

            REQUIRE_EQ(bad_status1, bad_status2);
        }

        SUBCASE(" TestComparePayloadsAfterErase") {
            turbo::Status payload_status(turbo::kInternal, "");
            payload_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
            payload_status.SetPayload(kUrl2, turbo::Cord(kPayload2));

            turbo::Status empty_status(turbo::kInternal, "");

            // Different payloads, not equal
            REQUIRE_NE(payload_status, empty_status);
            REQUIRE(payload_status.ErasePayload(kUrl1));

            // Still Different payloads, still not equal.
            REQUIRE_NE(payload_status, empty_status);
            REQUIRE(payload_status.ErasePayload(kUrl2));

            // Both empty payloads, should be equal
            REQUIRE_EQ(payload_status, empty_status);
        }


        /*
        SUBCASE(" TestForEachPayload") {
            turbo::Status bad_status(turbo::kInternal, "fail");
            bad_status.SetPayload(kUrl1, turbo::Cord(kPayload1));
            bad_status.SetPayload(kUrl2, turbo::Cord(kPayload2));
            bad_status.SetPayload(kUrl3, turbo::Cord(kPayload3));

            int count = 0;

            bad_status.ForEachPayload(
                    [&count](std::string_view, const turbo::Cord &) { ++count; });

            REQUIRE_EQ(count, 3);

            PayloadsVec expected_payloads = {{kUrl1, turbo::Cord(kPayload1)},
                                             {kUrl2, turbo::Cord(kPayload2)},
                                             {kUrl3, turbo::Cord(kPayload3)}};

            // Test that we visit all the payloads in the status.
            //PayloadsVec visited_payloads = AllVisitedPayloads(bad_status);
            //EXPECT_THAT(visited_payloads, UnorderedElementsAreArray(expected_payloads));

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
        }*/

        SUBCASE(" ToString") {
            turbo::Status s(turbo::kInternal, "fail");
            REQUIRE_EQ("INTERNAL: fail", s.ToString());
            s.SetPayload("foo", turbo::Cord("bar"));
            REQUIRE_EQ("INTERNAL: fail [foo='bar']", s.ToString());
            s.SetPayload("bar", turbo::Cord("\377"));
            std::cout<<s.ToString()<<std::endl;
            /*
            EXPECT_THAT(s.ToString(),
                        AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                              HasSubstr("[bar='\\xff']")));
                              */
        }

        SUBCASE(" ToStringMode") {
            turbo::Status s(turbo::kInternal, "fail");
            s.SetPayload("foo", turbo::Cord("bar"));
            s.SetPayload("bar", turbo::Cord("\377"));

            REQUIRE_EQ("INTERNAL: fail",
                       s.ToString(turbo::StatusToStringMode::kWithNoExtraData));
            /*
            EXPECT_THAT(s.ToString(turbo::StatusToStringMode::kWithPayload),
                        AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                              HasSubstr("[bar='\\xff']")));

            EXPECT_THAT(s.ToString(turbo::StatusToStringMode::kWithEverything),
                        AllOf(HasSubstr("INTERNAL: fail"), HasSubstr("[foo='bar']"),
                              HasSubstr("[bar='\\xff']")));

            EXPECT_THAT(s.ToString(~turbo::StatusToStringMode::kWithPayload),
                        AllOf(HasSubstr("INTERNAL: fail"), Not(HasSubstr("[foo='bar']")),
                              Not(HasSubstr("[bar='\\xff']"))));
                              */
        }

        SUBCASE(" CopyOnWriteForErasePayload") {
            {
                turbo::Status base(turbo::kInvalidArgument, "fail");
                base.SetPayload(kUrl1, turbo::Cord(kPayload1));
                REQUIRE(base.GetPayload(kUrl1).has_value());
                turbo::Status copy = EraseAndReturn(base);
                REQUIRE(base.GetPayload(kUrl1).has_value());
                REQUIRE_FALSE(copy.GetPayload(kUrl1).has_value());
            }
            {
                turbo::Status base(turbo::kInvalidArgument, "fail");
                base.SetPayload(kUrl1, turbo::Cord(kPayload1));
                turbo::Status copy = base;

                REQUIRE(base.GetPayload(kUrl1).has_value());
                REQUIRE(copy.GetPayload(kUrl1).has_value());

                REQUIRE(base.ErasePayload(kUrl1));

                REQUIRE_FALSE(base.GetPayload(kUrl1).has_value());
                REQUIRE(copy.GetPayload(kUrl1).has_value());
            }
        }

        SUBCASE(" CopyConstructor") {
            {
                turbo::Status status;
                turbo::Status copy(status);
                REQUIRE_EQ(copy, status);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                turbo::Status copy(status);
                REQUIRE_EQ(copy, status);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                status.SetPayload(kUrl1, turbo::Cord(kPayload1));
                turbo::Status copy(status);
                REQUIRE_EQ(copy, status);
            }
        }

        SUBCASE(" CopyAssignment") {
            turbo::Status assignee;
            {
                turbo::Status status;
                assignee = status;
                REQUIRE_EQ(assignee, status);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                assignee = status;
                REQUIRE_EQ(assignee, status);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                status.SetPayload(kUrl1, turbo::Cord(kPayload1));
                assignee = status;
                REQUIRE_EQ(assignee, status);
            }
        }

        SUBCASE(" CopyAssignmentIsNotRef") {
            const turbo::Status status_orig(turbo::kInvalidArgument, "message");
            turbo::Status status_copy = status_orig;
            REQUIRE_EQ(status_orig, status_copy);
            status_copy.SetPayload(kUrl1, turbo::Cord(kPayload1));
            REQUIRE_NE(status_orig, status_copy);
        }

        SUBCASE(" MoveConstructor") {
            {
                turbo::Status status;
                turbo::Status copy(turbo::Status{});
                REQUIRE_EQ(copy, status);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                turbo::Status copy(
                        turbo::Status(turbo::kInvalidArgument, "message"));
                REQUIRE_EQ(copy, status);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                status.SetPayload(kUrl1, turbo::Cord(kPayload1));
                turbo::Status copy1(status);
                turbo::Status copy2(std::move(status));
                REQUIRE_EQ(copy1, copy2);
            }
        }

        SUBCASE(" MoveAssignment") {
            turbo::Status assignee;
            {
                turbo::Status status;
                assignee = turbo::Status();
                REQUIRE_EQ(assignee, status);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                assignee = turbo::Status(turbo::kInvalidArgument, "message");
                REQUIRE_EQ(assignee, status);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                status.SetPayload(kUrl1, turbo::Cord(kPayload1));
                turbo::Status copy(status);
                assignee = std::move(status);
                REQUIRE_EQ(assignee, copy);
            }
            {
                turbo::Status status(turbo::kInvalidArgument, "message");
                turbo::Status copy(status);
                status = static_cast<turbo::Status &&>(status);
                REQUIRE_EQ(status, copy);
            }
        }

        SUBCASE(" Update") {
            turbo::Status s;
            s.Update(turbo::OkStatus());
            REQUIRE(s.ok());
            const turbo::Status a(turbo::kCancelled, "message");
            s.Update(a);
            REQUIRE_EQ(s, a);
            const turbo::Status b(turbo::kInternal, "other message");
            s.Update(b);
            REQUIRE_EQ(s, a);
            s.Update(turbo::OkStatus());
            REQUIRE_EQ(s, a);
            REQUIRE_FALSE(s.ok());
        }

        SUBCASE(" Equality") {
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
                        REQUIRE(status_arr[i] == status_arr[j]);
                        REQUIRE_FALSE(status_arr[i] != status_arr[j]);
                    } else {
                        REQUIRE(status_arr[i] != status_arr[j]);
                        REQUIRE_FALSE(status_arr[i] == status_arr[j]);
                    }
                }
            }
        }

        SUBCASE(" Swap") {
            auto test_swap = [](const turbo::Status &s1, const turbo::Status &s2) {
                turbo::Status copy1 = s1, copy2 = s2;
                swap(copy1, copy2);
                REQUIRE_EQ(copy1, s2);
                REQUIRE_EQ(copy2, s1);
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
    }
    TEST_CASE("StatusErrno") {
        SUBCASE("ErrnoToStatusCode") {
            REQUIRE_EQ(turbo::ErrnoToStatusCode(0), turbo::kOk);

            // Spot-check a few errno values.
            REQUIRE_EQ(turbo::ErrnoToStatusCode(EINVAL),
                       turbo::kInvalidArgument);
            REQUIRE_EQ(turbo::ErrnoToStatusCode(ENOENT), turbo::kNotFound);

            // We'll pick a very large number so it hopefully doesn't collide to errno.
            REQUIRE_EQ(turbo::ErrnoToStatusCode(19980927), turbo::kUnknown);
        }

        SUBCASE("ErrnoToStatus") {
            turbo::Status status = turbo::ErrnoToStatus(ENOENT, "Cannot open 'path'");
            REQUIRE_EQ(status.code(), turbo::kNotFound);
            REQUIRE_EQ(status.message(), "Cannot open 'path': No such file or directory");
        }
    }

}  // namespace
