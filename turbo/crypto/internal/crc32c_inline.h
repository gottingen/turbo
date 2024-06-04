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

#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/endian.h>
#include <turbo/crypto/internal/crc32_x86_arm_combined_simd.h>

namespace turbo::crc_internal {

    // CRC32C implementation optimized for small inputs.
    // Either computes crc and return true, or if there is
    // no hardware support does nothing and returns false.
    inline bool ExtendCrc32cInline(uint32_t *crc, const char *p, size_t n) {
#if defined(TURBO_CRC_INTERNAL_HAVE_ARM_SIMD) || \
    defined(TURBO_CRC_INTERNAL_HAVE_X86_SIMD)
        constexpr uint32_t kCrc32Xor = 0xffffffffU;
        *crc ^= kCrc32Xor;
        if (n & 1) {
          *crc = CRC32_u8(*crc, static_cast<uint8_t>(*p));
          n--;
          p++;
        }
        if (n & 2) {
          *crc = CRC32_u16(*crc, turbo::little_endian::load16(p));
          n -= 2;
          p += 2;
        }
        if (n & 4) {
          *crc = CRC32_u32(*crc, turbo::little_endian::load32(p));
          n -= 4;
          p += 4;
        }
        while (n) {
          *crc = CRC32_u64(*crc, turbo::little_endian::load64(p));
          n -= 8;
          p += 8;
        }
        *crc ^= kCrc32Xor;
        return true;
#else
        // No hardware support, signal the need to fallback.
        static_cast<void>(crc);
        static_cast<void>(p);
        static_cast<void>(n);
        return false;
#endif  // defined(TURBO_CRC_INTERNAL_HAVE_ARM_SIMD) ||
        // defined(TURBO_CRC_INTERNAL_HAVE_X86_SIMD)
    }

}  // namespace turbo::crc_internal
