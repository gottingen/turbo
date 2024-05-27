//
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

#include <turbo/log/check.h>

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

#include <gtest/gtest.h>
#include <tests/log/check_test_impl.inc>
