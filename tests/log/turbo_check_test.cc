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

#include <turbo/log/turbo_check.h>

#define TURBO_TEST_CHECK TURBO_CHECK
#define TURBO_TEST_CHECK_OK TURBO_CHECK_OK
#define TURBO_TEST_CHECK_EQ TURBO_CHECK_EQ
#define TURBO_TEST_CHECK_NE TURBO_CHECK_NE
#define TURBO_TEST_CHECK_GE TURBO_CHECK_GE
#define TURBO_TEST_CHECK_LE TURBO_CHECK_LE
#define TURBO_TEST_CHECK_GT TURBO_CHECK_GT
#define TURBO_TEST_CHECK_LT TURBO_CHECK_LT
#define TURBO_TEST_CHECK_STREQ TURBO_CHECK_STREQ
#define TURBO_TEST_CHECK_STRNE TURBO_CHECK_STRNE
#define TURBO_TEST_CHECK_STRCASEEQ TURBO_CHECK_STRCASEEQ
#define TURBO_TEST_CHECK_STRCASENE TURBO_CHECK_STRCASENE

#define TURBO_TEST_DCHECK TURBO_DCHECK
#define TURBO_TEST_DCHECK_OK TURBO_DCHECK_OK
#define TURBO_TEST_DCHECK_EQ TURBO_DCHECK_EQ
#define TURBO_TEST_DCHECK_NE TURBO_DCHECK_NE
#define TURBO_TEST_DCHECK_GE TURBO_DCHECK_GE
#define TURBO_TEST_DCHECK_LE TURBO_DCHECK_LE
#define TURBO_TEST_DCHECK_GT TURBO_DCHECK_GT
#define TURBO_TEST_DCHECK_LT TURBO_DCHECK_LT
#define TURBO_TEST_DCHECK_STREQ TURBO_DCHECK_STREQ
#define TURBO_TEST_DCHECK_STRNE TURBO_DCHECK_STRNE
#define TURBO_TEST_DCHECK_STRCASEEQ TURBO_DCHECK_STRCASEEQ
#define TURBO_TEST_DCHECK_STRCASENE TURBO_DCHECK_STRCASENE

#define TURBO_TEST_QCHECK TURBO_QCHECK
#define TURBO_TEST_QCHECK_OK TURBO_QCHECK_OK
#define TURBO_TEST_QCHECK_EQ TURBO_QCHECK_EQ
#define TURBO_TEST_QCHECK_NE TURBO_QCHECK_NE
#define TURBO_TEST_QCHECK_GE TURBO_QCHECK_GE
#define TURBO_TEST_QCHECK_LE TURBO_QCHECK_LE
#define TURBO_TEST_QCHECK_GT TURBO_QCHECK_GT
#define TURBO_TEST_QCHECK_LT TURBO_QCHECK_LT
#define TURBO_TEST_QCHECK_STREQ TURBO_QCHECK_STREQ
#define TURBO_TEST_QCHECK_STRNE TURBO_QCHECK_STRNE
#define TURBO_TEST_QCHECK_STRCASEEQ TURBO_QCHECK_STRCASEEQ
#define TURBO_TEST_QCHECK_STRCASENE TURBO_QCHECK_STRCASENE

#include <gtest/gtest.h>
#include <tests/log/check_test_impl.inc>
