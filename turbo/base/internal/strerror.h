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

#ifndef TURBO_BASE_INTERNAL_STRERROR_H_
#define TURBO_BASE_INTERNAL_STRERROR_H_

#include <string>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

// A portable and thread-safe alternative to C89's `strerror`.
//
// The C89 specification of `strerror` is not suitable for use in a
// multi-threaded application as the returned string may be changed by calls to
// `strerror` from another thread.  The many non-stdlib alternatives differ
// enough in their names, availability, and semantics to justify this wrapper
// around them.  `errno` will not be modified by a call to `turbo::StrError`.
std::string StrError(int errnum);

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_STRERROR_H_
