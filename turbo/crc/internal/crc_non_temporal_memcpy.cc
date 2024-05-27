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
#include <turbo/crc/crc32c.h>
#include <turbo/crc/internal/crc_memcpy.h>
#include <turbo/crc/internal/non_temporal_memcpy.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace crc_internal {

crc32c_t CrcNonTemporalMemcpyEngine::Compute(void* __restrict dst,
                                             const void* __restrict src,
                                             std::size_t length,
                                             crc32c_t initial_crc) const {
  constexpr size_t kBlockSize = 8192;
  crc32c_t crc = initial_crc;

  const char* src_bytes = reinterpret_cast<const char*>(src);
  char* dst_bytes = reinterpret_cast<char*>(dst);

  // Copy + CRC loop - run 8k chunks until we are out of full chunks.
  std::size_t offset = 0;
  for (; offset + kBlockSize < length; offset += kBlockSize) {
    crc = turbo::ExtendCrc32c(crc,
                             turbo::string_view(src_bytes + offset, kBlockSize));
    non_temporal_store_memcpy(dst_bytes + offset, src_bytes + offset,
                              kBlockSize);
  }

  // Save some work if length is 0.
  if (offset < length) {
    std::size_t final_copy_size = length - offset;
    crc = ExtendCrc32c(crc,
                       turbo::string_view(src_bytes + offset, final_copy_size));

    non_temporal_store_memcpy(dst_bytes + offset, src_bytes + offset,
                              final_copy_size);
  }

  return crc;
}

crc32c_t CrcNonTemporalMemcpyAVXEngine::Compute(void* __restrict dst,
                                                const void* __restrict src,
                                                std::size_t length,
                                                crc32c_t initial_crc) const {
  constexpr size_t kBlockSize = 8192;
  crc32c_t crc = initial_crc;

  const char* src_bytes = reinterpret_cast<const char*>(src);
  char* dst_bytes = reinterpret_cast<char*>(dst);

  // Copy + CRC loop - run 8k chunks until we are out of full chunks.
  std::size_t offset = 0;
  for (; offset + kBlockSize < length; offset += kBlockSize) {
    crc = ExtendCrc32c(crc, turbo::string_view(src_bytes + offset, kBlockSize));

    non_temporal_store_memcpy_avx(dst_bytes + offset, src_bytes + offset,
                                  kBlockSize);
  }

  // Save some work if length is 0.
  if (offset < length) {
    std::size_t final_copy_size = length - offset;
    crc = ExtendCrc32c(crc,
                       turbo::string_view(src_bytes + offset, final_copy_size));

    non_temporal_store_memcpy_avx(dst_bytes + offset, src_bytes + offset,
                                  final_copy_size);
  }

  return crc;
}

}  // namespace crc_internal
TURBO_NAMESPACE_END
}  // namespace turbo
