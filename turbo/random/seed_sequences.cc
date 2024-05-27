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

#include <turbo/random/seed_sequences.h>

#include <turbo/random/internal/pool_urbg.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

SeedSeq MakeSeedSeq() {
  SeedSeq::result_type seed_material[8];
  random_internal::RandenPool<uint32_t>::Fill(turbo::MakeSpan(seed_material));
  return SeedSeq(std::begin(seed_material), std::end(seed_material));
}

TURBO_NAMESPACE_END
}  // namespace turbo
