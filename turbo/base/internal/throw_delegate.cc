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

#include <turbo/base/internal/throw_delegate.h>

#include <cstdlib>
#include <functional>
#include <new>
#include <stdexcept>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

// NOTE: The exception types, like `std::logic_error`, do not exist on all
// platforms. (For example, the Android NDK does not have them.)
// Therefore, their use must be guarded by `#ifdef` or equivalent.

void ThrowStdLogicError(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::logic_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdLogicError(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::logic_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}
void ThrowStdInvalidArgument(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::invalid_argument(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdInvalidArgument(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::invalid_argument(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}

void ThrowStdDomainError(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::domain_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdDomainError(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::domain_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}

void ThrowStdLengthError(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::length_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdLengthError(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::length_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}

void ThrowStdOutOfRange(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::out_of_range(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdOutOfRange(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::out_of_range(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}

void ThrowStdRuntimeError(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::runtime_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdRuntimeError(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::runtime_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}

void ThrowStdRangeError(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::range_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdRangeError(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::range_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}

void ThrowStdOverflowError(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::overflow_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdOverflowError(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::overflow_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}

void ThrowStdUnderflowError(const std::string& what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::underflow_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg.c_str());
  std::abort();
#endif
}
void ThrowStdUnderflowError(const char* what_arg) {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::underflow_error(what_arg);
#else
  TURBO_RAW_LOG(FATAL, "%s", what_arg);
  std::abort();
#endif
}

void ThrowStdBadFunctionCall() {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::bad_function_call();
#else
  std::abort();
#endif
}

void ThrowStdBadAlloc() {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw std::bad_alloc();
#else
  std::abort();
#endif
}

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo
