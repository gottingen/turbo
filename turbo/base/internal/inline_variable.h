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

#ifndef TURBO_BASE_INTERNAL_INLINE_VARIABLE_H_
#define TURBO_BASE_INTERNAL_INLINE_VARIABLE_H_

#include <type_traits>

#include <turbo/base/internal/identity.h>

// File:
//   This file define a macro that allows the creation of or emulation of C++17
//   inline variables based on whether or not the feature is supported.

////////////////////////////////////////////////////////////////////////////////
// Macro: TURBO_INTERNAL_INLINE_CONSTEXPR(type, name, init)
//
// Description:
//   Expands to the equivalent of an inline constexpr instance of the specified
//   `type` and `name`, initialized to the value `init`. If the compiler being
//   used is detected as supporting actual inline variables as a language
//   feature, then the macro expands to an actual inline variable definition.
//
// Requires:
//   `type` is a type that is usable in an extern variable declaration.
//
// Requires: `name` is a valid identifier
//
// Requires:
//   `init` is an expression that can be used in the following definition:
//     constexpr type name = init;
//
// Usage:
//
//   // Equivalent to: `inline constexpr size_t variant_npos = -1;`
//   TURBO_INTERNAL_INLINE_CONSTEXPR(size_t, variant_npos, -1);
//
// Differences in implementation:
//   For a direct, language-level inline variable, decltype(name) will be the
//   type that was specified along with const qualification, whereas for
//   emulated inline variables, decltype(name) may be different (in practice
//   it will likely be a reference type).
////////////////////////////////////////////////////////////////////////////////

#ifdef __cpp_inline_variables

// Clang's -Wmissing-variable-declarations option erroneously warned that
// inline constexpr objects need to be pre-declared. This has now been fixed,
// but we will need to support this workaround for people building with older
// versions of clang.
//
// Bug: https://bugs.llvm.org/show_bug.cgi?id=35862
//
// Note:
//   type_identity_t is used here so that the const and name are in the
//   appropriate place for pointer types, reference types, function pointer
//   types, etc..
#if defined(__clang__)
#define TURBO_INTERNAL_EXTERN_DECL(type, name) \
  extern const ::turbo::internal::type_identity_t<type> name;
#else  // Otherwise, just define the macro to do nothing.
#define TURBO_INTERNAL_EXTERN_DECL(type, name)
#endif  // defined(__clang__)

// See above comment at top of file for details.
#define TURBO_INTERNAL_INLINE_CONSTEXPR(type, name, init) \
  TURBO_INTERNAL_EXTERN_DECL(type, name)                  \
  inline constexpr ::turbo::internal::type_identity_t<type> name = init

#else

// See above comment at top of file for details.
//
// Note:
//   type_identity_t is used here so that the const and name are in the
//   appropriate place for pointer types, reference types, function pointer
//   types, etc..
#define TURBO_INTERNAL_INLINE_CONSTEXPR(var_type, name, init)                 \
  template <class /*TurboInternalDummy*/ = void>                              \
  struct TurboInternalInlineVariableHolder##name {                            \
    static constexpr ::turbo::internal::type_identity_t<var_type> kInstance = \
        init;                                                                \
  };                                                                         \
                                                                             \
  template <class TurboInternalDummy>                                         \
  constexpr ::turbo::internal::type_identity_t<var_type>                      \
      TurboInternalInlineVariableHolder##name<TurboInternalDummy>::kInstance;  \
                                                                             \
  static constexpr const ::turbo::internal::type_identity_t<var_type>&        \
      name = /* NOLINT */                                                    \
      TurboInternalInlineVariableHolder##name<>::kInstance;                   \
  static_assert(sizeof(void (*)(decltype(name))) != 0,                       \
                "Silence unused variable warnings.")

#endif  // __cpp_inline_variables

#endif  // TURBO_BASE_INTERNAL_INLINE_VARIABLE_H_
