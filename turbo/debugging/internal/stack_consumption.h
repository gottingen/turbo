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

// Helper function for measuring stack consumption of signal handlers.

#ifndef TURBO_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_
#define TURBO_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_

#include <turbo/base/config.h>

// The code in this module is not portable.
// Use this feature test macro to detect its availability.
#ifdef TURBO_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION
#error TURBO_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION cannot be set directly
#elif !defined(__APPLE__) && !defined(_WIN32) &&                     \
    (defined(__i386__) || defined(__x86_64__) || defined(__ppc__) || \
     defined(__aarch64__) || defined(__riscv))
#define TURBO_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION 1

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

// Returns the stack consumption in bytes for the code exercised by
// signal_handler.  To measure stack consumption, signal_handler is registered
// as a signal handler, so the code that it exercises must be async-signal
// safe.  The argument of signal_handler is an implementation detail of signal
// handlers and should ignored by the code for signal_handler.  Use global
// variables to pass information between your test code and signal_handler.
int GetSignalHandlerStackConsumption(void (*signal_handler)(int));

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_HAVE_DEBUGGING_STACK_CONSUMPTION

#endif  // TURBO_DEBUGGING_INTERNAL_STACK_CONSUMPTION_H_
