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

#include <turbo/base/internal/throw_delegate.h>

#include <functional>
#include <new>
#include <stdexcept>

#include <turbo/base/config.h>
#include <gtest/gtest.h>

namespace {

using turbo::base_internal::ThrowStdLogicError;
using turbo::base_internal::ThrowStdInvalidArgument;
using turbo::base_internal::ThrowStdDomainError;
using turbo::base_internal::ThrowStdLengthError;
using turbo::base_internal::ThrowStdOutOfRange;
using turbo::base_internal::ThrowStdRuntimeError;
using turbo::base_internal::ThrowStdRangeError;
using turbo::base_internal::ThrowStdOverflowError;
using turbo::base_internal::ThrowStdUnderflowError;
using turbo::base_internal::ThrowStdBadFunctionCall;
using turbo::base_internal::ThrowStdBadAlloc;

constexpr const char* what_arg = "The quick brown fox jumps over the lazy dog";

template <typename E>
void ExpectThrowChar(void (*f)(const char*)) {
#ifdef TURBO_HAVE_EXCEPTIONS
  try {
    f(what_arg);
    FAIL() << "Didn't throw";
  } catch (const E& e) {
    EXPECT_STREQ(e.what(), what_arg);
  }
#else
  EXPECT_DEATH_IF_SUPPORTED(f(what_arg), what_arg);
#endif
}

template <typename E>
void ExpectThrowString(void (*f)(const std::string&)) {
#ifdef TURBO_HAVE_EXCEPTIONS
  try {
    f(what_arg);
    FAIL() << "Didn't throw";
  } catch (const E& e) {
    EXPECT_STREQ(e.what(), what_arg);
  }
#else
  EXPECT_DEATH_IF_SUPPORTED(f(what_arg), what_arg);
#endif
}

template <typename E>
void ExpectThrowNoWhat(void (*f)()) {
#ifdef TURBO_HAVE_EXCEPTIONS
  try {
    f();
    FAIL() << "Didn't throw";
  } catch (const E& e) {
  }
#else
  EXPECT_DEATH_IF_SUPPORTED(f(), "");
#endif
}

TEST(ThrowDelegate, ThrowStdLogicErrorChar) {
  ExpectThrowChar<std::logic_error>(ThrowStdLogicError);
}

TEST(ThrowDelegate, ThrowStdInvalidArgumentChar) {
  ExpectThrowChar<std::invalid_argument>(ThrowStdInvalidArgument);
}

TEST(ThrowDelegate, ThrowStdDomainErrorChar) {
  ExpectThrowChar<std::domain_error>(ThrowStdDomainError);
}

TEST(ThrowDelegate, ThrowStdLengthErrorChar) {
  ExpectThrowChar<std::length_error>(ThrowStdLengthError);
}

TEST(ThrowDelegate, ThrowStdOutOfRangeChar) {
  ExpectThrowChar<std::out_of_range>(ThrowStdOutOfRange);
}

TEST(ThrowDelegate, ThrowStdRuntimeErrorChar) {
  ExpectThrowChar<std::runtime_error>(ThrowStdRuntimeError);
}

TEST(ThrowDelegate, ThrowStdRangeErrorChar) {
  ExpectThrowChar<std::range_error>(ThrowStdRangeError);
}

TEST(ThrowDelegate, ThrowStdOverflowErrorChar) {
  ExpectThrowChar<std::overflow_error>(ThrowStdOverflowError);
}

TEST(ThrowDelegate, ThrowStdUnderflowErrorChar) {
  ExpectThrowChar<std::underflow_error>(ThrowStdUnderflowError);
}

TEST(ThrowDelegate, ThrowStdLogicErrorString) {
  ExpectThrowString<std::logic_error>(ThrowStdLogicError);
}

TEST(ThrowDelegate, ThrowStdInvalidArgumentString) {
  ExpectThrowString<std::invalid_argument>(ThrowStdInvalidArgument);
}

TEST(ThrowDelegate, ThrowStdDomainErrorString) {
  ExpectThrowString<std::domain_error>(ThrowStdDomainError);
}

TEST(ThrowDelegate, ThrowStdLengthErrorString) {
  ExpectThrowString<std::length_error>(ThrowStdLengthError);
}

TEST(ThrowDelegate, ThrowStdOutOfRangeString) {
  ExpectThrowString<std::out_of_range>(ThrowStdOutOfRange);
}

TEST(ThrowDelegate, ThrowStdRuntimeErrorString) {
  ExpectThrowString<std::runtime_error>(ThrowStdRuntimeError);
}

TEST(ThrowDelegate, ThrowStdRangeErrorString) {
  ExpectThrowString<std::range_error>(ThrowStdRangeError);
}

TEST(ThrowDelegate, ThrowStdOverflowErrorString) {
  ExpectThrowString<std::overflow_error>(ThrowStdOverflowError);
}

TEST(ThrowDelegate, ThrowStdUnderflowErrorString) {
  ExpectThrowString<std::underflow_error>(ThrowStdUnderflowError);
}

TEST(ThrowDelegate, ThrowStdBadFunctionCallNoWhat) {
#ifdef TURBO_HAVE_EXCEPTIONS
  try {
    ThrowStdBadFunctionCall();
    FAIL() << "Didn't throw";
  } catch (const std::bad_function_call&) {
  }
#ifdef _LIBCPP_VERSION
  catch (const std::exception&) {
    // https://reviews.llvm.org/D92397 causes issues with the vtable for
    // std::bad_function_call when using libc++ as a shared library.
  }
#endif
#else
  EXPECT_DEATH_IF_SUPPORTED(ThrowStdBadFunctionCall(), "");
#endif
}

TEST(ThrowDelegate, ThrowStdBadAllocNoWhat) {
  ExpectThrowNoWhat<std::bad_alloc>(ThrowStdBadAlloc);
}

}  // namespace
