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

#pragma once

#include <turbo/base/config.h>
#include <turbo/crypto/crc32c.h>

namespace turbo::crc_internal {

    // Modifies a CRC32 value by removing `length` bytes with a value of 0 from
    // the end of the string.
    //
    // This is the inverse operation of extend_crc32c_by_zeroes().
    //
    // This operation has a runtime cost of O(log(`length`))
    //
    // Internal implementation detail, exposed for testing only.
    CRC32C UnextendCrc32cByZeroes(CRC32C initial_crc, size_t length);

}  // namespace turbo::crc_internal
