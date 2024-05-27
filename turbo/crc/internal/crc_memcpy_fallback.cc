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

#include <cstring>
#include <memory>

#include <turbo/base/config.h>
#include <turbo/crc/crc32c.h>
#include <turbo/crc/internal/crc_memcpy.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace crc_internal {

turbo::crc32c_t FallbackCrcMemcpyEngine::Compute(void* __restrict dst,
                                                const void* __restrict src,
                                                std::size_t length,
                                                crc32c_t initial_crc) const {
  constexpr size_t kBlockSize = 8192;
  turbo::crc32c_t crc = initial_crc;

  const char* src_bytes = reinterpret_cast<const char*>(src);
  char* dst_bytes = reinterpret_cast<char*>(dst);

  // Copy + CRC loop - run 8k chunks until we are out of full chunks.  CRC
  // then copy was found to be slightly more efficient in our test cases.
  std::size_t offset = 0;
  for (; offset + kBlockSize < length; offset += kBlockSize) {
    crc = turbo::ExtendCrc32c(crc,
                             turbo::string_view(src_bytes + offset, kBlockSize));
    memcpy(dst_bytes + offset, src_bytes + offset, kBlockSize);
  }

  // Save some work if length is 0.
  if (offset < length) {
    std::size_t final_copy_size = length - offset;
    crc = turbo::ExtendCrc32c(
        crc, turbo::string_view(src_bytes + offset, final_copy_size));
    memcpy(dst_bytes + offset, src_bytes + offset, final_copy_size);
  }

  return crc;
}

// Compile the following only if we don't have
#if !defined(TURBO_INTERNAL_HAVE_X86_64_ACCELERATED_CRC_MEMCPY_ENGINE) && \
    !defined(TURBO_INTERNAL_HAVE_ARM_ACCELERATED_CRC_MEMCPY_ENGINE)

CrcMemcpy::ArchSpecificEngines CrcMemcpy::GetArchSpecificEngines() {
  CrcMemcpy::ArchSpecificEngines engines;
  engines.temporal = new FallbackCrcMemcpyEngine();
  engines.non_temporal = new FallbackCrcMemcpyEngine();
  return engines;
}

std::unique_ptr<CrcMemcpyEngine> CrcMemcpy::GetTestEngine(int /*vector*/,
                                                          int /*integer*/) {
  return std::make_unique<FallbackCrcMemcpyEngine>();
}

#endif  // !TURBO_INTERNAL_HAVE_X86_64_ACCELERATED_CRC_MEMCPY_ENGINE &&
        // !TURBO_INTERNAL_HAVE_ARM_ACCELERATED_CRC_MEMCPY_ENGINE

}  // namespace crc_internal
TURBO_NAMESPACE_END
}  // namespace turbo
