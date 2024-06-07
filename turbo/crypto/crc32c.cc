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

#include <turbo/crypto/crc32c.h>

#include <cstdint>

#include <turbo/crypto/internal/crc.h>
#include <turbo/crypto/internal/crc32c.h>
#include <turbo/crypto/internal/crc_memcpy.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    namespace {

        const crc_internal::CRC *CrcEngine() {
            static const crc_internal::CRC *engine = crc_internal::CRC::Crc32c();
            return engine;
        }

        constexpr uint32_t kCRC32Xor = 0xffffffffU;

    }  // namespace

    namespace crc_internal {

        CRC32C UnextendCrc32cByZeroes(CRC32C initial_crc, size_t length) {
            uint32_t crc = static_cast<uint32_t>(initial_crc) ^ kCRC32Xor;
            CrcEngine()->UnextendByZeroes(&crc, length);
            return static_cast<CRC32C>(crc ^ kCRC32Xor);
        }

        // Called by `turbo::extend_crc32c()` on strings with size > 64 or when hardware
        // CRC32C support is missing.
        CRC32C extend_crc32c_internal(CRC32C initial_crc,
                                        std::string_view buf_to_add) {
            uint32_t crc = static_cast<uint32_t>(initial_crc) ^ kCRC32Xor;
            CrcEngine()->Extend(&crc, buf_to_add.data(), buf_to_add.size());
            return static_cast<CRC32C>(crc ^ kCRC32Xor);
        }

    }  // namespace crc_internal

    CRC32C compute_crc32c(std::string_view buf) {
        return extend_crc32c(CRC32C{0}, buf);
    }

    CRC32C extend_crc32c_by_zeroes(CRC32C initial_crc, size_t length) {
        uint32_t crc = static_cast<uint32_t>(initial_crc) ^ kCRC32Xor;
        CrcEngine()->ExtendByZeroes(&crc, length);
        return static_cast<CRC32C>(crc ^ kCRC32Xor);
    }

    CRC32C concat_crc32c(CRC32C lhs_crc, CRC32C rhs_crc, size_t rhs_len) {
        uint32_t result = static_cast<uint32_t>(lhs_crc);
        CrcEngine()->ExtendByZeroes(&result, rhs_len);
        return CRC32C{result ^ static_cast<uint32_t>(rhs_crc)};
    }

    CRC32C remove_crc32c_prefix(CRC32C crc_a, CRC32C crc_ab, size_t length_b) {
        return concat_crc32c(crc_a, crc_ab, length_b);
    }

    CRC32C memcpy_crc32c(void *dest, const void *src, size_t count,
                           CRC32C initial_crc) {
        return static_cast<CRC32C>(
                crc_internal::Crc32CAndCopy(dest, src, count, initial_crc, false));
    }

    // Remove a Suffix of given size from a buffer
    //
    // Given a CRC32C of an existing buffer, `full_string_crc`; the CRC32C of a
    // suffix of that buffer to remove, `suffix_crc`; and suffix buffer's length,
    // `suffix_len` return the CRC32C of the buffer with suffix removed
    //
    // This operation has a runtime cost of O(log(`suffix_len`))
    CRC32C remove_crc32c_suffix(CRC32C full_string_crc, CRC32C suffix_crc,
                                  size_t suffix_len) {
        uint32_t result = static_cast<uint32_t>(full_string_crc) ^
                          static_cast<uint32_t>(suffix_crc);
        CrcEngine()->UnextendByZeroes(&result, suffix_len);
        return CRC32C{result};
    }

}  // namespace turbo
