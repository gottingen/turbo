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
//
// -----------------------------------------------------------------------------
// File: log/turbo_check.h
// -----------------------------------------------------------------------------
//
// This header declares a family of `TURBO_CHECK` macros as alternative spellings
// for `CHECK` macros in `check.h`.
//
// Except for those whose names begin with `TURBO_DCHECK`, these macros are not
// controlled by `NDEBUG` (cf. `assert`), so the check will be executed
// regardless of compilation mode. `TURBO_CHECK` and friends are thus useful for
// confirming invariants in situations where continuing to run would be worse
// than terminating, e.g., due to risk of data corruption or security
// compromise.  It is also more robust and portable to deliberately terminate
// at a particular place with a useful message and backtrace than to assume some
// ultimately unspecified and unreliable crashing behavior (such as a
// "segmentation fault").
//
// For full documentation of each macro, see comments in `check.h`, which has an
// identical set of macros without the TURBO_* prefix.

#ifndef TURBO_LOG_TURBO_CHECK_H_
#define TURBO_LOG_TURBO_CHECK_H_

#include <turbo/log/internal/check_impl.h>

#define TURBO_CHECK(condition) \
  TURBO_LOG_INTERNAL_CHECK_IMPL((condition), #condition)
#define TURBO_QCHECK(condition) \
  TURBO_LOG_INTERNAL_QCHECK_IMPL((condition), #condition)
#define TURBO_PCHECK(condition) \
  TURBO_LOG_INTERNAL_PCHECK_IMPL((condition), #condition)
#define TURBO_DCHECK(condition) \
  TURBO_LOG_INTERNAL_DCHECK_IMPL((condition), #condition)

#define TURBO_CHECK_EQ(val1, val2) \
  TURBO_LOG_INTERNAL_CHECK_EQ_IMPL((val1), #val1, (val2), #val2)
#define TURBO_CHECK_NE(val1, val2) \
  TURBO_LOG_INTERNAL_CHECK_NE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_CHECK_LE(val1, val2) \
  TURBO_LOG_INTERNAL_CHECK_LE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_CHECK_LT(val1, val2) \
  TURBO_LOG_INTERNAL_CHECK_LT_IMPL((val1), #val1, (val2), #val2)
#define TURBO_CHECK_GE(val1, val2) \
  TURBO_LOG_INTERNAL_CHECK_GE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_CHECK_GT(val1, val2) \
  TURBO_LOG_INTERNAL_CHECK_GT_IMPL((val1), #val1, (val2), #val2)
#define TURBO_QCHECK_EQ(val1, val2) \
  TURBO_LOG_INTERNAL_QCHECK_EQ_IMPL((val1), #val1, (val2), #val2)
#define TURBO_QCHECK_NE(val1, val2) \
  TURBO_LOG_INTERNAL_QCHECK_NE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_QCHECK_LE(val1, val2) \
  TURBO_LOG_INTERNAL_QCHECK_LE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_QCHECK_LT(val1, val2) \
  TURBO_LOG_INTERNAL_QCHECK_LT_IMPL((val1), #val1, (val2), #val2)
#define TURBO_QCHECK_GE(val1, val2) \
  TURBO_LOG_INTERNAL_QCHECK_GE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_QCHECK_GT(val1, val2) \
  TURBO_LOG_INTERNAL_QCHECK_GT_IMPL((val1), #val1, (val2), #val2)
#define TURBO_DCHECK_EQ(val1, val2) \
  TURBO_LOG_INTERNAL_DCHECK_EQ_IMPL((val1), #val1, (val2), #val2)
#define TURBO_DCHECK_NE(val1, val2) \
  TURBO_LOG_INTERNAL_DCHECK_NE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_DCHECK_LE(val1, val2) \
  TURBO_LOG_INTERNAL_DCHECK_LE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_DCHECK_LT(val1, val2) \
  TURBO_LOG_INTERNAL_DCHECK_LT_IMPL((val1), #val1, (val2), #val2)
#define TURBO_DCHECK_GE(val1, val2) \
  TURBO_LOG_INTERNAL_DCHECK_GE_IMPL((val1), #val1, (val2), #val2)
#define TURBO_DCHECK_GT(val1, val2) \
  TURBO_LOG_INTERNAL_DCHECK_GT_IMPL((val1), #val1, (val2), #val2)

#define TURBO_CHECK_OK(status) TURBO_LOG_INTERNAL_CHECK_OK_IMPL((status), #status)
#define TURBO_QCHECK_OK(status) \
  TURBO_LOG_INTERNAL_QCHECK_OK_IMPL((status), #status)
#define TURBO_DCHECK_OK(status) \
  TURBO_LOG_INTERNAL_DCHECK_OK_IMPL((status), #status)

#define TURBO_CHECK_STREQ(s1, s2) \
  TURBO_LOG_INTERNAL_CHECK_STREQ_IMPL((s1), #s1, (s2), #s2)
#define TURBO_CHECK_STRNE(s1, s2) \
  TURBO_LOG_INTERNAL_CHECK_STRNE_IMPL((s1), #s1, (s2), #s2)
#define TURBO_CHECK_STRCASEEQ(s1, s2) \
  TURBO_LOG_INTERNAL_CHECK_STRCASEEQ_IMPL((s1), #s1, (s2), #s2)
#define TURBO_CHECK_STRCASENE(s1, s2) \
  TURBO_LOG_INTERNAL_CHECK_STRCASENE_IMPL((s1), #s1, (s2), #s2)
#define TURBO_QCHECK_STREQ(s1, s2) \
  TURBO_LOG_INTERNAL_QCHECK_STREQ_IMPL((s1), #s1, (s2), #s2)
#define TURBO_QCHECK_STRNE(s1, s2) \
  TURBO_LOG_INTERNAL_QCHECK_STRNE_IMPL((s1), #s1, (s2), #s2)
#define TURBO_QCHECK_STRCASEEQ(s1, s2) \
  TURBO_LOG_INTERNAL_QCHECK_STRCASEEQ_IMPL((s1), #s1, (s2), #s2)
#define TURBO_QCHECK_STRCASENE(s1, s2) \
  TURBO_LOG_INTERNAL_QCHECK_STRCASENE_IMPL((s1), #s1, (s2), #s2)
#define TURBO_DCHECK_STREQ(s1, s2) \
  TURBO_LOG_INTERNAL_DCHECK_STREQ_IMPL((s1), #s1, (s2), #s2)
#define TURBO_DCHECK_STRNE(s1, s2) \
  TURBO_LOG_INTERNAL_DCHECK_STRNE_IMPL((s1), #s1, (s2), #s2)
#define TURBO_DCHECK_STRCASEEQ(s1, s2) \
  TURBO_LOG_INTERNAL_DCHECK_STRCASEEQ_IMPL((s1), #s1, (s2), #s2)
#define TURBO_DCHECK_STRCASENE(s1, s2) \
  TURBO_LOG_INTERNAL_DCHECK_STRCASENE_IMPL((s1), #s1, (s2), #s2)

#endif  // TURBO_LOG_TURBO_CHECK_H_
