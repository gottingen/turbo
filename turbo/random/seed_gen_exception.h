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
// File: seed_gen_exception.h
// -----------------------------------------------------------------------------
//
// This header defines an exception class which may be thrown if unpredictable
// events prevent the derivation of suitable seed-material for constructing a
// bit generator conforming to [rand.req.urng] (eg. entropy cannot be read from
// /dev/urandom on a Unix-based system).
//
// Note: if exceptions are disabled, `std::terminate()` is called instead.

#ifndef TURBO_RANDOM_SEED_GEN_EXCEPTION_H_
#define TURBO_RANDOM_SEED_GEN_EXCEPTION_H_

#include <exception>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// SeedGenException
//------------------------------------------------------------------------------
class SeedGenException : public std::exception {
 public:
  SeedGenException() = default;
  ~SeedGenException() override;
  const char* what() const noexcept override;
};

namespace random_internal {

// throw delegator
[[noreturn]] void ThrowSeedGenException();

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_SEED_GEN_EXCEPTION_H_
