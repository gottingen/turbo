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

#ifndef TURBO_BASE_INTERNAL_THROW_DELEGATE_H_
#define TURBO_BASE_INTERNAL_THROW_DELEGATE_H_

#include <string>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

// Helper functions that allow throwing exceptions consistently from anywhere.
// The main use case is for header-based libraries (eg templates), as they will
// be built by many different targets with their own compiler options.
// In particular, this will allow a safe way to throw exceptions even if the
// caller is compiled with -fno-exceptions.  This is intended for implementing
// things like map<>::at(), which the standard documents as throwing an
// exception on error.
//
// Using other techniques like #if tricks could lead to ODR violations.
//
// You shouldn't use it unless you're writing code that you know will be built
// both with and without exceptions and you need to conform to an interface
// that uses exceptions.

[[noreturn]] void ThrowStdLogicError(const std::string& what_arg);
[[noreturn]] void ThrowStdLogicError(const char* what_arg);
[[noreturn]] void ThrowStdInvalidArgument(const std::string& what_arg);
[[noreturn]] void ThrowStdInvalidArgument(const char* what_arg);
[[noreturn]] void ThrowStdDomainError(const std::string& what_arg);
[[noreturn]] void ThrowStdDomainError(const char* what_arg);
[[noreturn]] void ThrowStdLengthError(const std::string& what_arg);
[[noreturn]] void ThrowStdLengthError(const char* what_arg);
[[noreturn]] void ThrowStdOutOfRange(const std::string& what_arg);
[[noreturn]] void ThrowStdOutOfRange(const char* what_arg);
[[noreturn]] void ThrowStdRuntimeError(const std::string& what_arg);
[[noreturn]] void ThrowStdRuntimeError(const char* what_arg);
[[noreturn]] void ThrowStdRangeError(const std::string& what_arg);
[[noreturn]] void ThrowStdRangeError(const char* what_arg);
[[noreturn]] void ThrowStdOverflowError(const std::string& what_arg);
[[noreturn]] void ThrowStdOverflowError(const char* what_arg);
[[noreturn]] void ThrowStdUnderflowError(const std::string& what_arg);
[[noreturn]] void ThrowStdUnderflowError(const char* what_arg);

[[noreturn]] void ThrowStdBadFunctionCall();
[[noreturn]] void ThrowStdBadAlloc();

// ThrowStdBadArrayNewLength() cannot be consistently supported because
// std::bad_array_new_length is missing in libstdc++ until 4.9.0.
// https://gcc.gnu.org/onlinedocs/gcc-4.8.3/libstdc++/api/a01379_source.html
// https://gcc.gnu.org/onlinedocs/gcc-4.9.0/libstdc++/api/a01327_source.html
// libcxx (as of 3.2) and msvc (as of 2015) both have it.
// [[noreturn]] void ThrowStdBadArrayNewLength();

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_THROW_DELEGATE_H_
