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

#include <turbo/random/seed_gen_exception.h>

#include <iostream>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

static constexpr const char kExceptionMessage[] =
    "Failed generating seed-material for URBG.";

SeedGenException::~SeedGenException() = default;

const char* SeedGenException::what() const noexcept {
  return kExceptionMessage;
}

namespace random_internal {

void ThrowSeedGenException() {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw turbo::SeedGenException();
#else
  std::cerr << kExceptionMessage << std::endl;
  std::terminate();
#endif
}

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo
