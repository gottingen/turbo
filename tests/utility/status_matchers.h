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

#ifndef TURBO_STATUS_INTERNAL_STATUS_MATCHERS_H_
#define TURBO_STATUS_INTERNAL_STATUS_MATCHERS_H_

#include <ostream>  // NOLINT
#include <string>
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>  // gmock_for_status_matchers.h
#include <turbo/base/config.h>
#include <turbo/utility/status_impl.h>
#include <turbo/utility/result_impl.h>
#include <turbo/strings/string_view.h>

namespace turbo_testing {
TURBO_NAMESPACE_BEGIN
namespace status_internal {

inline const turbo::Status& GetStatus(const turbo::Status& status) {
  return status;
}

template <typename T>
inline const turbo::Status& GetStatus(const turbo::Result<T>& status) {
  return status.status();
}

////////////////////////////////////////////////////////////
// Implementation of IsOkAndHolds().

// Monomorphic implementation of matcher IsOkAndHolds(m).  StatusOrType is a
// reference to Result<T>.
template <typename StatusOrType>
class IsOkAndHoldsMatcherImpl
    : public ::testing::MatcherInterface<StatusOrType> {
 public:
  typedef
      typename std::remove_reference<StatusOrType>::type::value_type value_type;

  template <typename InnerMatcher>
  explicit IsOkAndHoldsMatcherImpl(InnerMatcher&& inner_matcher)
      : inner_matcher_(::testing::SafeMatcherCast<const value_type&>(
            std::forward<InnerMatcher>(inner_matcher))) {}

  void DescribeTo(std::ostream* os) const override {
    *os << "is OK and has a value that ";
    inner_matcher_.DescribeTo(os);
  }

  void DescribeNegationTo(std::ostream* os) const override {
    *os << "isn't OK or has a value that ";
    inner_matcher_.DescribeNegationTo(os);
  }

  bool MatchAndExplain(
      StatusOrType actual_value,
      ::testing::MatchResultListener* result_listener) const override {
    if (!actual_value.ok()) {
      *result_listener << "which has status " << actual_value.status();
      return false;
    }

    // Call through to the inner matcher.
    return inner_matcher_.MatchAndExplain(*actual_value, result_listener);
  }

 private:
  const ::testing::Matcher<const value_type&> inner_matcher_;
};

// Implements IsOkAndHolds(m) as a polymorphic matcher.
template <typename InnerMatcher>
class IsOkAndHoldsMatcher {
 public:
  explicit IsOkAndHoldsMatcher(InnerMatcher inner_matcher)
      : inner_matcher_(std::forward<InnerMatcher>(inner_matcher)) {}

  // Converts this polymorphic matcher to a monomorphic matcher of the
  // given type.  StatusOrType can be either Result<T> or a
  // reference to Result<T>.
  template <typename StatusOrType>
  operator ::testing::Matcher<StatusOrType>() const {  // NOLINT
    return ::testing::Matcher<StatusOrType>(
        new IsOkAndHoldsMatcherImpl<const StatusOrType&>(inner_matcher_));
  }

 private:
  const InnerMatcher inner_matcher_;
};

////////////////////////////////////////////////////////////
// Implementation of StatusIs().

// `StatusCode` is implicitly convertible from `int`, `turbo::StatusCode`, and
//  is explicitly convertible to these types as well.
//
// We need this class because `turbo::StatusCode` (as a scoped enum) is not
// implicitly convertible to `int`. In order to handle use cases like
// ```
// StatusIs(Anyof(turbo::StatusCode::kUnknown, turbo::StatusCode::kCancelled))
// ```
// which uses polymorphic matchers, we need to unify the interfaces into
// `Matcher<StatusCode>`.
class StatusCode {
 public:
  /*implicit*/ StatusCode(int code)  // NOLINT
      : code_(static_cast<::turbo::StatusCode>(code)) {}
  /*implicit*/ StatusCode(::turbo::StatusCode code) : code_(code) {}  // NOLINT

  explicit operator int() const { return static_cast<int>(code_); }

  friend inline void PrintTo(const StatusCode& code, std::ostream* os) {
    // TODO(b/321095377): Change this to print the status code as a string.
    *os << static_cast<int>(code);
  }

 private:
  ::turbo::StatusCode code_;
};

// Relational operators to handle matchers like Eq, Lt, etc..
inline bool operator==(const StatusCode& lhs, const StatusCode& rhs) {
  return static_cast<int>(lhs) == static_cast<int>(rhs);
}
inline bool operator!=(const StatusCode& lhs, const StatusCode& rhs) {
  return static_cast<int>(lhs) != static_cast<int>(rhs);
}

// StatusIs() is a polymorphic matcher.  This class is the common
// implementation of it shared by all types T where StatusIs() can be
// used as a Matcher<T>.
class StatusIsMatcherCommonImpl {
 public:
  StatusIsMatcherCommonImpl(
      ::testing::Matcher<StatusCode> code_matcher,
      ::testing::Matcher<turbo::string_view> message_matcher)
      : code_matcher_(std::move(code_matcher)),
        message_matcher_(std::move(message_matcher)) {}

  void DescribeTo(std::ostream* os) const;

  void DescribeNegationTo(std::ostream* os) const;

  bool MatchAndExplain(const turbo::Status& status,
                       ::testing::MatchResultListener* result_listener) const;

 private:
  const ::testing::Matcher<StatusCode> code_matcher_;
  const ::testing::Matcher<turbo::string_view> message_matcher_;
};

// Monomorphic implementation of matcher StatusIs() for a given type
// T.  T can be Status, Result<>, or a reference to either of them.
template <typename T>
class MonoStatusIsMatcherImpl : public ::testing::MatcherInterface<T> {
 public:
  explicit MonoStatusIsMatcherImpl(StatusIsMatcherCommonImpl common_impl)
      : common_impl_(std::move(common_impl)) {}

  void DescribeTo(std::ostream* os) const override {
    common_impl_.DescribeTo(os);
  }

  void DescribeNegationTo(std::ostream* os) const override {
    common_impl_.DescribeNegationTo(os);
  }

  bool MatchAndExplain(
      T actual_value,
      ::testing::MatchResultListener* result_listener) const override {
    return common_impl_.MatchAndExplain(GetStatus(actual_value),
                                        result_listener);
  }

 private:
  StatusIsMatcherCommonImpl common_impl_;
};

// Implements StatusIs() as a polymorphic matcher.
class StatusIsMatcher {
 public:
  template <typename StatusCodeMatcher, typename StatusMessageMatcher>
  StatusIsMatcher(StatusCodeMatcher&& code_matcher,
                  StatusMessageMatcher&& message_matcher)
      : common_impl_(::testing::MatcherCast<StatusCode>(
                         std::forward<StatusCodeMatcher>(code_matcher)),
                     ::testing::MatcherCast<turbo::string_view>(
                         std::forward<StatusMessageMatcher>(message_matcher))) {
  }

  // Converts this polymorphic matcher to a monomorphic matcher of the
  // given type.  T can be Result<>, Status, or a reference to
  // either of them.
  template <typename T>
  /*implicit*/ operator ::testing::Matcher<T>() const {  // NOLINT
    return ::testing::Matcher<T>(
        new MonoStatusIsMatcherImpl<const T&>(common_impl_));
  }

 private:
  const StatusIsMatcherCommonImpl common_impl_;
};

// Monomorphic implementation of matcher IsOk() for a given type T.
// T can be Status, Result<>, or a reference to either of them.
template <typename T>
class MonoIsOkMatcherImpl : public ::testing::MatcherInterface<T> {
 public:
  void DescribeTo(std::ostream* os) const override { *os << "is OK"; }
  void DescribeNegationTo(std::ostream* os) const override {
    *os << "is not OK";
  }
  bool MatchAndExplain(T actual_value,
                       ::testing::MatchResultListener*) const override {
    return GetStatus(actual_value).ok();
  }
};

// Implements IsOk() as a polymorphic matcher.
class IsOkMatcher {
 public:
  template <typename T>
  /*implicit*/ operator ::testing::Matcher<T>() const {  // NOLINT
    return ::testing::Matcher<T>(new MonoIsOkMatcherImpl<const T&>());
  }
};

}  // namespace status_internal
TURBO_NAMESPACE_END
}  // namespace turbo_testing

#endif  // TURBO_STATUS_INTERNAL_STATUS_MATCHERS_H_
