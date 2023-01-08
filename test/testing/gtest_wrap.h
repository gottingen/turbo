
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_GTEST_WRAP_H
#define FLARE_GTEST_WRAP_H

#ifdef private
#undef private
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#define private public
#else
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#endif

#endif //FLARE_GTEST_WRAP_H
