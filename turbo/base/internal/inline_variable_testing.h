// Copyright 2022 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_BASE_INTERNAL_INLINE_VARIABLE_TESTING_H_
#define TURBO_BASE_INTERNAL_INLINE_VARIABLE_TESTING_H_

#include "turbo/base/internal/inline_variable.h"

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
