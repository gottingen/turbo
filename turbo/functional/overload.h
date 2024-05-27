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
// File: overload.h
// -----------------------------------------------------------------------------
//
// `turbo::Overload` is a functor that provides overloads based on the functors
// with which it is created. This can, for example, be used to locally define an
// anonymous visitor type for `std::visit` inside a function using lambdas.
//
// Before using this function, consider whether named function overloads would
// be a better design.
//
// Note: turbo::Overload requires C++17.
//
// Example:
//
//     std::variant<std::string, int32_t, int64_t> v(int32_t{1});
//     const size_t result =
//         std::visit(turbo::Overload{
//                        [](const std::string& s) { return s.size(); },
//                        [](const auto& s) { return sizeof(s); },
//                    },
//                    v);
//     assert(result == 4);
//

#ifndef TURBO_FUNCTIONAL_OVERLOAD_H_
#define TURBO_FUNCTIONAL_OVERLOAD_H_

#include <turbo/base/config.h>
#include <turbo/meta/type_traits.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

#if defined(TURBO_INTERNAL_CPLUSPLUS_LANG) && \
    TURBO_INTERNAL_CPLUSPLUS_LANG >= 201703L

template <typename... T>
struct Overload final : T... {
  using T::operator()...;

  // For historical reasons we want to support use that looks like a function
  // call:
  //
  //     turbo::Overload(lambda_1, lambda_2)
  //
  // This works automatically in C++20 because we have support for parenthesized
  // aggregate initialization. Before then we must provide a constructor that
  // makes this work.
  //
  constexpr explicit Overload(T... ts) : T(std::move(ts))... {}
};

// Before C++20, which added support for CTAD for aggregate types, we must also
// teach the compiler how to deduce the template arguments for Overload.
//
template <typename... T>
Overload(T...) -> Overload<T...>;

#else

namespace functional_internal {
template <typename T>
constexpr bool kDependentFalse = false;
}

template <typename Dependent = int, typename... T>
auto Overload(T&&...) {
  static_assert(functional_internal::kDependentFalse<Dependent>,
                "Overload is only usable with C++17 or above.");
}

#endif

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FUNCTIONAL_OVERLOAD_H_
