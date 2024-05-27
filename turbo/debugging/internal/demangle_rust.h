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

#ifndef TURBO_DEBUGGING_INTERNAL_DEMANGLE_RUST_H_
#define TURBO_DEBUGGING_INTERNAL_DEMANGLE_RUST_H_

#include <cstddef>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

// Demangle the Rust encoding `mangled`.  On success, return true and write the
// demangled symbol name to `out`.  Otherwise, return false, leaving unspecified
// contents in `out`.  For example, calling DemangleRustSymbolEncoding with
// `mangled = "_RNvC8my_crate7my_func"` will yield `my_crate::my_func` in `out`,
// provided `out_size` is large enough for that value and its trailing NUL.
//
// DemangleRustSymbolEncoding is async-signal-safe and runs in bounded C++
// call-stack space.  It is suitable for symbolizing stack traces in a signal
// handler.
//
// The demangling logic is under development; search for "not yet implemented"
// in the .cc file to see where the gaps are.
bool DemangleRustSymbolEncoding(const char* mangled, char* out,
                                std::size_t out_size);

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_DEBUGGING_INTERNAL_DEMANGLE_RUST_H_
