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

#include <cstddef>

#include <turbo/base/config.h>
#include <turbo/crypto/crc32c.h>
#include <turbo/crypto/internal/crc_memcpy.h>
#include <turbo/crypto/internal/non_temporal_memcpy.h>
#include <turbo/strings/string_view.h>

namespace turbo::crc_internal {

    CRC32C CrcNonTemporalMemcpyEngine::Compute(void *__restrict dst,
                                               const void *__restrict src,
                                               std::size_t length,
                                               CRC32C initial_crc) const {
        constexpr size_t kBlockSize = 8192;
        CRC32C crc = initial_crc;

        const char *src_bytes = reinterpret_cast<const char *>(src);
        char *dst_bytes = reinterpret_cast<char *>(dst);

        // Copy + CRC loop - run 8k chunks until we are out of full chunks.
        std::size_t offset = 0;
        for (; offset + kBlockSize < length; offset += kBlockSize) {
            crc = turbo::extend_crc32c(crc,
                                       std::string_view(src_bytes + offset, kBlockSize));
            non_temporal_store_memcpy(dst_bytes + offset, src_bytes + offset,
                                      kBlockSize);
        }

        // Save some work if length is 0.
        if (offset < length) {
            std::size_t final_copy_size = length - offset;
            crc = extend_crc32c(crc,
                                std::string_view(src_bytes + offset, final_copy_size));

            non_temporal_store_memcpy(dst_bytes + offset, src_bytes + offset,
                                      final_copy_size);
        }

        return crc;
    }

    CRC32C CrcNonTemporalMemcpyAVXEngine::Compute(void *__restrict dst,
                                                  const void *__restrict src,
                                                  std::size_t length,
                                                  CRC32C initial_crc) const {
        constexpr size_t kBlockSize = 8192;
        CRC32C crc = initial_crc;

        const char *src_bytes = reinterpret_cast<const char *>(src);
        char *dst_bytes = reinterpret_cast<char *>(dst);

        // Copy + CRC loop - run 8k chunks until we are out of full chunks.
        std::size_t offset = 0;
        for (; offset + kBlockSize < length; offset += kBlockSize) {
            crc = extend_crc32c(crc, std::string_view(src_bytes + offset, kBlockSize));

            non_temporal_store_memcpy_avx(dst_bytes + offset, src_bytes + offset,
                                          kBlockSize);
        }

        // Save some work if length is 0.
        if (offset < length) {
            std::size_t final_copy_size = length - offset;
            crc = extend_crc32c(crc,
                                std::string_view(src_bytes + offset, final_copy_size));

            non_temporal_store_memcpy_avx(dst_bytes + offset, src_bytes + offset,
                                          final_copy_size);
        }

        return crc;
    }

}  // namespace turbo::crc_internal
