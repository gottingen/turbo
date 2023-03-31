// Copyright 2019 The Turbo Authors.
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
//
// -----------------------------------------------------------------------------
// File: options.h
// -----------------------------------------------------------------------------
//
// This file contains Turbo configuration options for setting specific
// implementations instead of letting Turbo determine which implementation to
// use at compile-time. Setting these options may be useful for package or build
// managers who wish to guarantee ABI stability within binary builds (which are
// otherwise difficult to enforce).
//
// *** IMPORTANT NOTICE FOR PACKAGE MANAGERS:  It is important that
// maintainers of package managers who wish to package Turbo read and
// understand this file! ***
//
// Turbo contains a number of possible configuration endpoints, based on
// parameters such as the detected platform, language version, or command-line
// flags used to invoke the underlying binary. As is the case with all
// libraries, binaries which contain Turbo code must ensure that separate
// packages use the same compiled copy of Turbo to avoid a diamond dependency
// problem, which can occur if two packages built with different Turbo
// configuration settings are linked together. Diamond dependency problems in
// C++ may manifest as violations to the One Definition Rule (ODR) (resulting in
// linker errors), or undefined behavior (resulting in crashes).
//
// Diamond dependency problems can be avoided if all packages utilize the same
// exact version of Turbo. Building from source code with the same compilation
// parameters is the easiest way to avoid such dependency problems. However, for
// package managers who cannot control such compilation parameters, we are
// providing the file to allow you to inject ABI (Application Binary Interface)
// stability across builds. Settings options in this file will neither change
// API nor ABI, providing a stable copy of Turbo between packages.
//
// Care must be taken to keep options within these configurations isolated
// from any other dynamic settings, such as command-line flags which could alter
// these options. This file is provided specifically to help build and package
// managers provide a stable copy of Turbo within their libraries and binaries;
// other developers should not have need to alter the contents of this file.
//
// -----------------------------------------------------------------------------
// Usage
// -----------------------------------------------------------------------------
//
// For any particular package release, set the appropriate definitions within
// this file to whatever value makes the most sense for your package(s). Note
// that, by default, most of these options, at the moment, affect the
// implementation of types; future options may affect other implementation
// details.
//
// NOTE: the defaults within this file all assume that Turbo can select the
// proper Turbo implementation at compile-time, which will not be sufficient
// to guarantee ABI stability to package managers.

#ifndef TURBO_PLATFORM_OPTIONS_H_
#define TURBO_PLATFORM_OPTIONS_H_

// -----------------------------------------------------------------------------
// Type Compatibility Options
// -----------------------------------------------------------------------------
//
// TURBO_OPTION_USE_STD_ANY
//
// This option controls whether turbo::any is implemented as an alias to
// std::any, or as an independent implementation.
//
// A value of 0 means to use Turbo's implementation.  This requires only C++11
// support, and is expected to work on every toolchain we support.
//
// A value of 1 means to use an alias to std::any.  This requires that all code
// using Turbo is built in C++17 mode or later.
//
// A value of 2 means to detect the C++ version being used to compile Turbo,
// and use an alias only if a working std::any is available.  This option is
// useful when you are building your entire program, including all of its
// dependencies, from source.  It should not be used otherwise -- for example,
// if you are distributing Turbo in a binary package manager -- since in
// mode 2, turbo::any will name a different type, with a different mangled name
// and binary layout, depending on the compiler flags passed by the end user.
// For more info, see https://abseil.io/about/design/dropin-types.
//
// User code should not inspect this macro.  To check in the preprocessor if
// turbo::any is a typedef of std::any, use the feature macro TURBO_USES_STD_ANY.

#define TURBO_OPTION_USE_STD_ANY 2


// TURBO_OPTION_USE_STD_OPTIONAL
//
// This option controls whether turbo::optional is implemented as an alias to
// std::optional, or as an independent implementation.
//
// A value of 0 means to use Turbo's implementation.  This requires only C++11
// support, and is expected to work on every toolchain we support.
//
// A value of 1 means to use an alias to std::optional.  This requires that all
// code using Turbo is built in C++17 mode or later.
//
// A value of 2 means to detect the C++ version being used to compile Turbo,
// and use an alias only if a working std::optional is available.  This option
// is useful when you are building your program from source.  It should not be
// used otherwise -- for example, if you are distributing Turbo in a binary
// package manager -- since in mode 2, turbo::optional will name a different
// type, with a different mangled name and binary layout, depending on the
// compiler flags passed by the end user.  For more info, see
// https://abseil.io/about/design/dropin-types.

// User code should not inspect this macro.  To check in the preprocessor if
// turbo::optional is a typedef of std::optional, use the feature macro
// TURBO_USES_STD_OPTIONAL.

#define TURBO_OPTION_USE_STD_OPTIONAL 2


// TURBO_OPTION_USE_STD_VARIANT
//
// This option controls whether turbo::variant is implemented as an alias to
// std::variant, or as an independent implementation.
//
// A value of 0 means to use Turbo's implementation.  This requires only C++11
// support, and is expected to work on every toolchain we support.
//
// A value of 1 means to use an alias to std::variant.  This requires that all
// code using Turbo is built in C++17 mode or later.
//
// A value of 2 means to detect the C++ version being used to compile Turbo,
// and use an alias only if a working std::variant is available.  This option
// is useful when you are building your program from source.  It should not be
// used otherwise -- for example, if you are distributing Turbo in a binary
// package manager -- since in mode 2, turbo::variant will name a different
// type, with a different mangled name and binary layout, depending on the
// compiler flags passed by the end user.  For more info, see
// https://abseil.io/about/design/dropin-types.
//
// User code should not inspect this macro.  To check in the preprocessor if
// turbo::variant is a typedef of std::variant, use the feature macro
// TURBO_USES_STD_VARIANT.

#define TURBO_OPTION_USE_STD_VARIANT 2


// TURBO_OPTION_USE_INLINE_NAMESPACE
// TURBO_OPTION_INLINE_NAMESPACE_NAME
//
// These options controls whether all entities in the turbo namespace are
// contained within an inner inline namespace.  This does not affect the
// user-visible API of Turbo, but it changes the mangled names of all symbols.
//
// This can be useful as a version tag if you are distributing Turbo in
// precompiled form.  This will prevent a binary library build of Turbo with
// one inline namespace being used with headers configured with a different
// inline namespace name.  Binary packagers are reminded that Turbo does not
// guarantee any ABI stability in Turbo, so any update of Turbo or
// configuration change in such a binary package should be combined with a
// new, unique value for the inline namespace name.
//
// A value of 0 means not to use inline namespaces.
//
// A value of 1 means to use an inline namespace with the given name inside
// namespace turbo.  If this is set, TURBO_OPTION_INLINE_NAMESPACE_NAME must also
// be changed to a new, unique identifier name.  In particular "head" is not
// allowed.

#define TURBO_OPTION_USE_INLINE_NAMESPACE 0
#define TURBO_OPTION_INLINE_NAMESPACE_NAME head

// TURBO_OPTION_HARDENED
//
// This option enables a "hardened" build in release mode (in this context,
// release mode is defined as a build where the `NDEBUG` macro is defined).
//
// A value of 0 means that "hardened" mode is not enabled.
//
// A value of 1 means that "hardened" mode is enabled.
//
// Hardened builds have additional security checks enabled when `NDEBUG` is
// defined. Defining `NDEBUG` is normally used to turn `assert()` macro into a
// no-op, as well as disabling other bespoke program consistency checks. By
// defining TURBO_OPTION_HARDENED to 1, a select set of checks remain enabled in
// release mode. These checks guard against programming errors that may lead to
// security vulnerabilities. In release mode, when one of these programming
// errors is encountered, the program will immediately abort, possibly without
// any attempt at logging.
//
// The checks enabled by this option are not free; they do incur runtime cost.
//
// The checks enabled by this option are always active when `NDEBUG` is not
// defined, even in the case when TURBO_OPTION_HARDENED is defined to 0. The
// checks enabled by this option may abort the program in a different way and
// log additional information when `NDEBUG` is not defined.

#define TURBO_OPTION_HARDENED 0

#endif  // TURBO_PLATFORM_OPTIONS_H_
