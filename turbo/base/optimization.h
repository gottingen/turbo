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
// File: optimization.h
// -----------------------------------------------------------------------------
//
// This header file defines portable macros for performance optimization.

#ifndef TURBO_BASE_OPTIMIZATION_H_
#define TURBO_BASE_OPTIMIZATION_H_

#include <assert.h>

#include <turbo/base/config.h>
#include <turbo/base/options.h>
#include <turbo/base/macros/likely.h>
#include <turbo/base/macros/assume.h>
#include <turbo/base/macros/cache_line.h>

// TURBO_BLOCK_TAIL_CALL_OPTIMIZATION
//
// Instructs the compiler to avoid optimizing tail-call recursion. This macro is
// useful when you wish to preserve the existing function order within a stack
// trace for logging, debugging, or profiling purposes.
//
// Example:
//
//   int f() {
//     int result = g();
//     TURBO_BLOCK_TAIL_CALL_OPTIMIZATION();
//     return result;
//   }
#if defined(__pnacl__)
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#elif defined(__clang__)
// Clang will not tail call given inline volatile assembly.
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(__GNUC__)
// GCC will not tail call given inline volatile assembly.
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() __asm__ __volatile__("")
#elif defined(_MSC_VER)
#include <intrin.h>
// The __nop() intrinsic blocks the optimisation.
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() __nop()
#else
#define TURBO_BLOCK_TAIL_CALL_OPTIMIZATION() if (volatile int x = 0) { (void)x; }
#endif

// TURBO_INTERNAL_UNIQUE_SMALL_NAME(cond)
// This macro forces small unique name on a static file level symbols like
// static local variables or static functions. This is intended to be used in
// macro definitions to optimize the cost of generated code. Do NOT use it on
// symbols exported from translation unit since it may cause a link time
// conflict.
//
// Example:
//
// #define MY_MACRO(txt)
// namespace {
//  char VeryVeryLongVarName[] TURBO_INTERNAL_UNIQUE_SMALL_NAME() = txt;
//  const char* VeryVeryLongFuncName() TURBO_INTERNAL_UNIQUE_SMALL_NAME();
//  const char* VeryVeryLongFuncName() { return txt; }
// }
//

#if defined(__GNUC__)
#define TURBO_INTERNAL_UNIQUE_SMALL_NAME2(x) #x
#define TURBO_INTERNAL_UNIQUE_SMALL_NAME1(x) TURBO_INTERNAL_UNIQUE_SMALL_NAME2(x)
#define TURBO_INTERNAL_UNIQUE_SMALL_NAME() \
  asm(TURBO_INTERNAL_UNIQUE_SMALL_NAME1(.turbo.__COUNTER__))
#else
#define TURBO_INTERNAL_UNIQUE_SMALL_NAME()
#endif

#endif  // TURBO_BASE_OPTIMIZATION_H_
