// Copyright 2023 The Turbo Authors.
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

#ifndef SIMDUTF_COMPILER_CHECK_H
#define SIMDUTF_COMPILER_CHECK_H


#ifndef SIMDUTF_CPLUSPLUS
#if defined(_MSVC_LANG) && !defined(__clang__)
#define SIMDUTF_CPLUSPLUS (_MSC_VER == 1900 ? 201103L : _MSVC_LANG)
#else
#define SIMDUTF_CPLUSPLUS __cplusplus
#endif
#endif

// C++ 17
#if !defined(SIMDUTF_CPLUSPLUS17) && (SIMDUTF_CPLUSPLUS >= 201703L)
#define SIMDUTF_CPLUSPLUS17 1
#endif

// C++ 14
#if !defined(SIMDUTF_CPLUSPLUS14) && (SIMDUTF_CPLUSPLUS >= 201402L)
#define SIMDUTF_CPLUSPLUS14 1
#endif

// C++ 11
#if !defined(SIMDUTF_CPLUSPLUS11) && (SIMDUTF_CPLUSPLUS >= 201103L)
#define SIMDUTF_CPLUSPLUS11 1
#endif

#ifndef SIMDUTF_CPLUSPLUS11
#error simdutf requires a compiler compliant with the C++11 standard
#endif

#endif // SIMDUTF_COMPILER_CHECK_H
