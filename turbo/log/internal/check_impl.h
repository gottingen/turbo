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

#ifndef TURBO_LOG_INTERNAL_CHECK_IMPL_H_
#define TURBO_LOG_INTERNAL_CHECK_IMPL_H_

#include <turbo/base/optimization.h>
#include <turbo/log/internal/check_op.h>
#include <turbo/log/internal/conditions.h>
#include <turbo/log/internal/log_message.h>
#include <turbo/log/internal/strip.h>

// CHECK
#define TURBO_LOG_INTERNAL_CHECK_IMPL(condition, condition_text)       \
  TURBO_LOG_INTERNAL_CONDITION_FATAL(STATELESS,                        \
                                    TURBO_UNLIKELY(!(condition))) \
  TURBO_LOG_INTERNAL_CHECK(condition_text).InternalStream()

#define TURBO_LOG_INTERNAL_QCHECK_IMPL(condition, condition_text)       \
  TURBO_LOG_INTERNAL_CONDITION_QFATAL(STATELESS,                        \
                                     TURBO_UNLIKELY(!(condition))) \
  TURBO_LOG_INTERNAL_QCHECK(condition_text).InternalStream()

#define TURBO_LOG_INTERNAL_PCHECK_IMPL(condition, condition_text) \
  TURBO_LOG_INTERNAL_CHECK_IMPL(condition, condition_text).WithPerror()

#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DCHECK_IMPL(condition, condition_text) \
  TURBO_LOG_INTERNAL_CHECK_IMPL(condition, condition_text)
#else
#define TURBO_LOG_INTERNAL_DCHECK_IMPL(condition, condition_text) \
  TURBO_LOG_INTERNAL_CHECK_IMPL(true || (condition), "true")
#endif

// CHECK_EQ
#define TURBO_LOG_INTERNAL_CHECK_EQ_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_OP(Check_EQ, ==, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_CHECK_NE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_OP(Check_NE, !=, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_CHECK_LE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_OP(Check_LE, <=, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_CHECK_LT_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_OP(Check_LT, <, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_CHECK_GE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_OP(Check_GE, >=, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_CHECK_GT_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_OP(Check_GT, >, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_QCHECK_EQ_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_QCHECK_OP(Check_EQ, ==, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_QCHECK_NE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_QCHECK_OP(Check_NE, !=, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_QCHECK_LE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_QCHECK_OP(Check_LE, <=, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_QCHECK_LT_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_QCHECK_OP(Check_LT, <, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_QCHECK_GE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_QCHECK_OP(Check_GE, >=, val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_QCHECK_GT_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_QCHECK_OP(Check_GT, >, val1, val1_text, val2, val2_text)
#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DCHECK_EQ_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_EQ_IMPL(val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_DCHECK_NE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_NE_IMPL(val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_DCHECK_LE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_LE_IMPL(val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_DCHECK_LT_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_LT_IMPL(val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_DCHECK_GE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_GE_IMPL(val1, val1_text, val2, val2_text)
#define TURBO_LOG_INTERNAL_DCHECK_GT_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_CHECK_GT_IMPL(val1, val1_text, val2, val2_text)
#else  // ndef NDEBUG
#define TURBO_LOG_INTERNAL_DCHECK_EQ_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(val1, val2)
#define TURBO_LOG_INTERNAL_DCHECK_NE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(val1, val2)
#define TURBO_LOG_INTERNAL_DCHECK_LE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(val1, val2)
#define TURBO_LOG_INTERNAL_DCHECK_LT_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(val1, val2)
#define TURBO_LOG_INTERNAL_DCHECK_GE_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(val1, val2)
#define TURBO_LOG_INTERNAL_DCHECK_GT_IMPL(val1, val1_text, val2, val2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(val1, val2)
#endif  // def NDEBUG

// CHECK_OK
#define TURBO_LOG_INTERNAL_CHECK_OK_IMPL(status, status_text) \
  TURBO_LOG_INTERNAL_CHECK_OK(status, status_text)
#define TURBO_LOG_INTERNAL_QCHECK_OK_IMPL(status, status_text) \
  TURBO_LOG_INTERNAL_QCHECK_OK(status, status_text)
#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DCHECK_OK_IMPL(status, status_text) \
  TURBO_LOG_INTERNAL_CHECK_OK(status, status_text)
#else
#define TURBO_LOG_INTERNAL_DCHECK_OK_IMPL(status, status_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(status, nullptr)
#endif

// CHECK_STREQ
#define TURBO_LOG_INTERNAL_CHECK_STREQ_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_CHECK_STROP(strcmp, ==, true, s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_CHECK_STRNE_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_CHECK_STROP(strcmp, !=, false, s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_CHECK_STRCASEEQ_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_CHECK_STROP(strcasecmp, ==, true, s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_CHECK_STRCASENE_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_CHECK_STROP(strcasecmp, !=, false, s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_QCHECK_STREQ_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_QCHECK_STROP(strcmp, ==, true, s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_QCHECK_STRNE_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_QCHECK_STROP(strcmp, !=, false, s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_QCHECK_STRCASEEQ_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_QCHECK_STROP(strcasecmp, ==, true, s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_QCHECK_STRCASENE_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_QCHECK_STROP(strcasecmp, !=, false, s1, s1_text, s2,  \
                                 s2_text)
#ifndef NDEBUG
#define TURBO_LOG_INTERNAL_DCHECK_STREQ_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_CHECK_STREQ_IMPL(s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_DCHECK_STRCASEEQ_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_CHECK_STRCASEEQ_IMPL(s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_DCHECK_STRNE_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_CHECK_STRNE_IMPL(s1, s1_text, s2, s2_text)
#define TURBO_LOG_INTERNAL_DCHECK_STRCASENE_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_CHECK_STRCASENE_IMPL(s1, s1_text, s2, s2_text)
#else  // ndef NDEBUG
#define TURBO_LOG_INTERNAL_DCHECK_STREQ_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(s1, s2)
#define TURBO_LOG_INTERNAL_DCHECK_STRCASEEQ_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(s1, s2)
#define TURBO_LOG_INTERNAL_DCHECK_STRNE_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(s1, s2)
#define TURBO_LOG_INTERNAL_DCHECK_STRCASENE_IMPL(s1, s1_text, s2, s2_text) \
  TURBO_LOG_INTERNAL_DCHECK_NOP(s1, s2)
#endif  // def NDEBUG

#endif  // TURBO_LOG_INTERNAL_CHECK_IMPL_H_
