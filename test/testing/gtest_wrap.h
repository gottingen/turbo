
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_GTEST_WRAP_H
#define TURBO_GTEST_WRAP_H

#ifdef private
#undef private
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#define private public
#else
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#endif

#endif //TURBO_GTEST_WRAP_H
