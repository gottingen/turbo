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

#ifndef TURBO_BASE_INTERNAL_INLINE_VARIABLE_TESTING_H_
#define TURBO_BASE_INTERNAL_INLINE_VARIABLE_TESTING_H_

#include <turbo/base/internal/inline_variable.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace inline_variable_testing_internal {

struct Foo {
  int value = 5;
};

TURBO_INTERNAL_INLINE_CONSTEXPR(Foo, inline_variable_foo, {});
TURBO_INTERNAL_INLINE_CONSTEXPR(Foo, other_inline_variable_foo, {});

TURBO_INTERNAL_INLINE_CONSTEXPR(int, inline_variable_int, 5);
TURBO_INTERNAL_INLINE_CONSTEXPR(int, other_inline_variable_int, 5);

TURBO_INTERNAL_INLINE_CONSTEXPR(void(*)(), inline_variable_fun_ptr, nullptr);

const Foo& get_foo_a();
const Foo& get_foo_b();

const int& get_int_a();
const int& get_int_b();

}  // namespace inline_variable_testing_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_INLINE_VARIABLE_TESTING_H_
