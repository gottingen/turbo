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

// Testing utilities for turbo types which throw exceptions.

#ifndef TURBO_BASE_INTERNAL_EXCEPTION_TESTING_H_
#define TURBO_BASE_INTERNAL_EXCEPTION_TESTING_H_

#include <gtest/gtest.h>
#include <turbo/base/config.h>

// TURBO_BASE_INTERNAL_EXPECT_FAIL tests either for a specified thrown exception
// if exceptions are enabled, or for death with a specified text in the error
// message
#ifdef TURBO_HAVE_EXCEPTIONS

#define TURBO_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_THROW(expr, exception_t)

#elif defined(__ANDROID__)
// Android asserts do not log anywhere that gtest can currently inspect.
// So we expect exit, but cannot match the message.
#define TURBO_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_DEATH(expr, ".*")
#else
#define TURBO_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_DEATH_IF_SUPPORTED(expr, text)

#endif

#endif  // TURBO_BASE_INTERNAL_EXCEPTION_TESTING_H_
