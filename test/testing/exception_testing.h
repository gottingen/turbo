
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TEST_TRESTING_EXCEPTION_TESTING_H_
#define TEST_TRESTING_EXCEPTION_TESTING_H_

#include "testing/gtest_wrap.h"
#include "flare/base/profile.h"

// FLARE_BASE_INTERNAL_EXPECT_FAIL tests either for a specified thrown exception
// if exceptions are enabled, or for death with a specified text in the error
// message
#ifdef FLARE_HAVE_EXCEPTIONS

#define FLARE_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_THROW(expr, exception_t)

#elif defined(__ANDROID__)
// Android asserts do not log anywhere that gtest can currently inspect.
// So we expect exit, but cannot match the message.
#define FLARE_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_DEATH(expr, ".*")
#else
#define FLARE_BASE_INTERNAL_EXPECT_FAIL(expr, exception_t, text) \
  EXPECT_DEATH_IF_SUPPORTED(expr, text)

#endif

#endif  // TEST_TRESTING_EXCEPTION_TESTING_H_
