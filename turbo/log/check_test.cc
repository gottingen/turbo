//
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

#include "turbo/log/check.h"

#define TURBO_TEST_CHECK CHECK
#define TURBO_TEST_CHECK_OK CHECK_OK
#define TURBO_TEST_CHECK_EQ CHECK_EQ
#define TURBO_TEST_CHECK_NE CHECK_NE
#define TURBO_TEST_CHECK_GE CHECK_GE
#define TURBO_TEST_CHECK_LE CHECK_LE
#define TURBO_TEST_CHECK_GT CHECK_GT
#define TURBO_TEST_CHECK_LT CHECK_LT
#define TURBO_TEST_CHECK_STREQ CHECK_STREQ
#define TURBO_TEST_CHECK_STRNE CHECK_STRNE
#define TURBO_TEST_CHECK_STRCASEEQ CHECK_STRCASEEQ
#define TURBO_TEST_CHECK_STRCASENE CHECK_STRCASENE

#define TURBO_TEST_DCHECK DCHECK
#define TURBO_TEST_DCHECK_OK DCHECK_OK
#define TURBO_TEST_DCHECK_EQ DCHECK_EQ
#define TURBO_TEST_DCHECK_NE DCHECK_NE
#define TURBO_TEST_DCHECK_GE DCHECK_GE
#define TURBO_TEST_DCHECK_LE DCHECK_LE
#define TURBO_TEST_DCHECK_GT DCHECK_GT
#define TURBO_TEST_DCHECK_LT DCHECK_LT
#define TURBO_TEST_DCHECK_STREQ DCHECK_STREQ
#define TURBO_TEST_DCHECK_STRNE DCHECK_STRNE
#define TURBO_TEST_DCHECK_STRCASEEQ DCHECK_STRCASEEQ
#define TURBO_TEST_DCHECK_STRCASENE DCHECK_STRCASENE

#define TURBO_TEST_QCHECK QCHECK
#define TURBO_TEST_QCHECK_OK QCHECK_OK
#define TURBO_TEST_QCHECK_EQ QCHECK_EQ
#define TURBO_TEST_QCHECK_NE QCHECK_NE
#define TURBO_TEST_QCHECK_GE QCHECK_GE
#define TURBO_TEST_QCHECK_LE QCHECK_LE
#define TURBO_TEST_QCHECK_GT QCHECK_GT
#define TURBO_TEST_QCHECK_LT QCHECK_LT
#define TURBO_TEST_QCHECK_STREQ QCHECK_STREQ
#define TURBO_TEST_QCHECK_STRNE QCHECK_STRNE
#define TURBO_TEST_QCHECK_STRCASEEQ QCHECK_STRCASEEQ
#define TURBO_TEST_QCHECK_STRCASENE QCHECK_STRCASENE

#include "gtest/gtest.h"
#include "turbo/log/check_test_impl.h"
