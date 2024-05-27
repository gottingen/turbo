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

#ifndef TURBO_DEBUGGING_INTERNAL_DEMANGLE_H_
#define TURBO_DEBUGGING_INTERNAL_DEMANGLE_H_

#include <string>
#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace debugging_internal {

// Demangle `mangled`.  On success, return true and write the
// demangled symbol name to `out`.  Otherwise, return false.
// `out` is modified even if demangling is unsuccessful.
//
// This function provides an alternative to libstdc++'s abi::__cxa_demangle,
// which is not async signal safe (it uses malloc internally).  It's intended to
// be used in async signal handlers to symbolize stack traces.
//
// Note that this demangler doesn't support full demangling.  More
// specifically, it doesn't print types of function parameters and
// types of template arguments.  It just skips them.  However, it's
// still very useful to extract basic information such as class,
// function, constructor, destructor, and operator names.
//
// See the implementation note in demangle.cc if you are interested.
//
// Example:
//
// | Mangled Name  | Demangle    | DemangleString
// |---------------|-------------|-----------------------
// | _Z1fv         | f()         | f()
// | _Z1fi         | f()         | f(int)
// | _Z3foo3bar    | foo()       | foo(bar)
// | _Z1fIiEvi     | f<>()       | void f<int>(int)
// | _ZN1N1fE      | N::f        | N::f
// | _ZN3Foo3BarEv | Foo::Bar()  | Foo::Bar()
// | _Zrm1XS_"     | operator%() | operator%(X, X)
// | _ZN3FooC1Ev   | Foo::Foo()  | Foo::Foo()
// | _Z1fSs        | f()         | f(std::basic_string<char,
// |               |             |   std::char_traits<char>,
// |               |             |   std::allocator<char> >)
//
// See the unit test for more examples.
//
// Support for Rust mangled names is in development; see demangle_rust.h.
//
// Note: we might want to write demanglers for ABIs other than Itanium
// C++ ABI in the future.
bool Demangle(const char* mangled, char* out, size_t out_size);

// A wrapper around `abi::__cxa_demangle()`.  On success, returns the demangled
// name.  On failure, returns the input mangled name.
//
// This function is not async-signal-safe.
std::string DemangleString(const char* mangled);

}  // namespace debugging_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_DEBUGGING_INTERNAL_DEMANGLE_H_
