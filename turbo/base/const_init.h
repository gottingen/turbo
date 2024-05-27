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
// kConstInit
// -----------------------------------------------------------------------------
//
// A constructor tag used to mark an object as safe for use as a global
// variable, avoiding the usual lifetime issues that can affect globals.

#ifndef TURBO_BASE_CONST_INIT_H_
#define TURBO_BASE_CONST_INIT_H_

#include <turbo/base/config.h>

// In general, objects with static storage duration (such as global variables)
// can trigger tricky object lifetime situations.  Attempting to access them
// from the constructors or destructors of other global objects can result in
// undefined behavior, unless their constructors and destructors are designed
// with this issue in mind.
//
// The normal way to deal with this issue in C++11 is to use constant
// initialization and trivial destructors.
//
// Constant initialization is guaranteed to occur before any other code
// executes.  Constructors that are declared 'constexpr' are eligible for
// constant initialization.  You can annotate a variable declaration with the
// TURBO_CONST_INIT macro to express this intent.  For compilers that support
// it, this annotation will cause a compilation error for declarations that
// aren't subject to constant initialization (perhaps because a runtime value
// was passed as a constructor argument).
//
// On program shutdown, lifetime issues can be avoided on global objects by
// ensuring that they contain  trivial destructors.  A class has a trivial
// destructor unless it has a user-defined destructor, a virtual method or base
// class, or a data member or base class with a non-trivial destructor of its
// own.  Objects with static storage duration and a trivial destructor are not
// cleaned up on program shutdown, and are thus safe to access from other code
// running during shutdown.
//
// For a few core Turbo classes, we make a best effort to allow for safe global
// instances, even though these classes have non-trivial destructors.  These
// objects can be created with the turbo::kConstInit tag.  For example:
//   TURBO_CONST_INIT turbo::Mutex global_mutex(turbo::kConstInit);
//
// The line above declares a global variable of type turbo::Mutex which can be
// accessed at any point during startup or shutdown.  global_mutex's destructor
// will still run, but will not invalidate the object.  Note that C++ specifies
// that accessing an object after its destructor has run results in undefined
// behavior, but this pattern works on the toolchains we support.
//
// The turbo::kConstInit tag should only be used to define objects with static
// or thread_local storage duration.

namespace turbo {
TURBO_NAMESPACE_BEGIN

enum ConstInitType {
  kConstInit,
};

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_CONST_INIT_H_
