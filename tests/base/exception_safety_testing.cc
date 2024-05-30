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

#include <tests/base/exception_safety_testing.h>

#ifdef TURBO_HAVE_EXCEPTIONS

#include <gtest/gtest.h>
#include <turbo/meta/type_traits.h>

namespace testing {

exceptions_internal::NoThrowTag nothrow_ctor;

exceptions_internal::StrongGuaranteeTagType strong_guarantee;

exceptions_internal::ExceptionSafetyTestBuilder<> MakeExceptionSafetyTester() {
  return {};
}

namespace exceptions_internal {

int countdown = -1;

ConstructorTracker* ConstructorTracker::current_tracker_instance_ = nullptr;

void MaybeThrow(turbo::string_view msg, bool throw_bad_alloc) {
  if (countdown-- == 0) {
    if (throw_bad_alloc) throw TestBadAllocException(msg);
    throw TestException(msg);
  }
}

testing::AssertionResult FailureMessage(const TestException& e,
                                        int countdown) noexcept {
  return testing::AssertionFailure() << "Exception thrown from " << e.what();
}

std::string GetSpecString(TypeSpec spec) {
  std::string out;
  turbo::string_view sep;
  const auto append = [&](turbo::string_view s) {
    turbo::str_append(&out, sep, s);
    sep = " | ";
  };
  if (static_cast<bool>(TypeSpec::kNoThrowCopy & spec)) {
    append("kNoThrowCopy");
  }
  if (static_cast<bool>(TypeSpec::kNoThrowMove & spec)) {
    append("kNoThrowMove");
  }
  if (static_cast<bool>(TypeSpec::kNoThrowNew & spec)) {
    append("kNoThrowNew");
  }
  return out;
}

std::string GetSpecString(AllocSpec spec) {
  return static_cast<bool>(AllocSpec::kNoThrowAllocate & spec)
             ? "kNoThrowAllocate"
             : "";
}

}  // namespace exceptions_internal

}  // namespace testing

#endif  // TURBO_HAVE_EXCEPTIONS
