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
// File: status_matchers.h
// -----------------------------------------------------------------------------
//
// Testing utilities for working with `turbo::Status` and `turbo::StatusOr`.
//
// Defines the following utilities:
//
//   ===============
//   `IsOkAndHolds(m)`
//   ===============
//
//   This gMock matcher matches a StatusOr<T> value whose status is OK
//   and whose inner value matches matcher m.  Example:
//
//   ```
//   using ::testing::MatchesRegex;
//   using ::turbo_testing::IsOkAndHolds;
//   ...
//   turbo::StatusOr<string> maybe_name = ...;
//   EXPECT_THAT(maybe_name, IsOkAndHolds(MatchesRegex("John .*")));
//   ```
//
//   ===============================
//   `StatusIs(status_code_matcher)`
//   ===============================
//
//   This is a shorthand for
//     `StatusIs(status_code_matcher, ::testing::_)`
//   In other words, it's like the two-argument `StatusIs()`, except that it
//   ignores error message.
//
//   ===============
//   `IsOk()`
//   ===============
//
//   Matches an `turbo::Status` or `turbo::StatusOr<T>` value whose status value
//   is `turbo::StatusCode::kOk.`
//
//   Equivalent to 'StatusIs(turbo::StatusCode::kOk)'.
//   Example:
//   ```
//   using ::turbo_testing::IsOk;
//   ...
//   turbo::StatusOr<string> maybe_name = ...;
//   EXPECT_THAT(maybe_name, IsOk());
//   Status s = ...;
//   EXPECT_THAT(s, IsOk());
//   ```

#ifndef TURBO_STATUS_STATUS_MATCHERS_H_
#define TURBO_STATUS_STATUS_MATCHERS_H_

#include <ostream>  // NOLINT
#include <type_traits>
#include <utility>

#include <gmock/gmock.h>  // gmock_for_status_matchers.h
#include <turbo/base/config.h>
#include <tests/status/status_matchers.h>

namespace turbo_testing {
    TURBO_NAMESPACE_BEGIN

    // Returns a gMock matcher that matches a StatusOr<> whose status is
    // OK and whose value matches the inner matcher.
    template<typename InnerMatcherT>
    status_internal::IsOkAndHoldsMatcher<typename std::decay<InnerMatcherT>::type>
    IsOkAndHolds(InnerMatcherT &&inner_matcher) {
        return status_internal::IsOkAndHoldsMatcher<
                typename std::decay<InnerMatcherT>::type>(
                std::forward<InnerMatcherT>(inner_matcher));
    }

    // Returns a gMock matcher that matches a Status or StatusOr<> whose status code
    // matches code_matcher and whose error message matches message_matcher.
    // Typically, code_matcher will be an turbo::StatusCode, e.g.
    //
    // StatusIs(turbo::StatusCode::kInvalidArgument, "...")
    template<typename StatusCodeMatcherT, typename StatusMessageMatcherT>
    status_internal::StatusIsMatcher StatusIs(
            StatusCodeMatcherT &&code_matcher,
            StatusMessageMatcherT &&message_matcher) {
        return status_internal::StatusIsMatcher(
                std::forward<StatusCodeMatcherT>(code_matcher),
                std::forward<StatusMessageMatcherT>(message_matcher));
    }

    // Returns a gMock matcher that matches a Status or StatusOr<> and whose status
    // code matches code_matcher.  See above for details.
    template<typename StatusCodeMatcherT>
    status_internal::StatusIsMatcher StatusIs(StatusCodeMatcherT &&code_matcher) {
        return StatusIs(std::forward<StatusCodeMatcherT>(code_matcher), ::testing::_);
    }

    // Returns a gMock matcher that matches a Status or StatusOr<> which is OK.
    inline status_internal::IsOkMatcher IsOk() {
        return status_internal::IsOkMatcher();
    }

    TURBO_NAMESPACE_END
}  // namespace turbo_testing

#endif  // TURBO_STATUS_STATUS_MATCHERS_H_
