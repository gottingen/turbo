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

#ifndef TURBO_CRC_INTERNAL_CRC_MEMCPY_H_
#define TURBO_CRC_INTERNAL_CRC_MEMCPY_H_

#include <cstddef>
#include <memory>

#include <turbo/base/config.h>
#include <turbo/crc/crc32c.h>
#include <turbo/crc/internal/crc32_x86_arm_combined_simd.h>

// Defined if the class AcceleratedCrcMemcpyEngine exists.
// TODO(b/299127771): Consider relaxing the pclmul requirement once the other
// intrinsics are conditionally compiled without it.
#if defined(TURBO_CRC_INTERNAL_HAVE_X86_SIMD)
#define TURBO_INTERNAL_HAVE_X86_64_ACCELERATED_CRC_MEMCPY_ENGINE 1
#elif defined(TURBO_CRC_INTERNAL_HAVE_ARM_SIMD)
#define TURBO_INTERNAL_HAVE_ARM_ACCELERATED_CRC_MEMCPY_ENGINE 1
#endif

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace crc_internal {

class CrcMemcpyEngine {
 public:
  virtual ~CrcMemcpyEngine() = default;

  virtual crc32c_t Compute(void* __restrict dst, const void* __restrict src,
                           std::size_t length, crc32c_t initial_crc) const = 0;

 protected:
  CrcMemcpyEngine() = default;
};

class CrcMemcpy {
 public:
  static crc32c_t CrcAndCopy(void* __restrict dst, const void* __restrict src,
                             std::size_t length,
                             crc32c_t initial_crc = crc32c_t{0},
                             bool non_temporal = false) {
    static const ArchSpecificEngines engines = GetArchSpecificEngines();
    auto* engine = non_temporal ? engines.non_temporal : engines.temporal;
    return engine->Compute(dst, src, length, initial_crc);
  }

  // For testing only: get an architecture-specific engine for tests.
  static std::unique_ptr<CrcMemcpyEngine> GetTestEngine(int vector,
                                                        int integer);

 private:
  struct ArchSpecificEngines {
    CrcMemcpyEngine* temporal;
    CrcMemcpyEngine* non_temporal;
  };

  static ArchSpecificEngines GetArchSpecificEngines();
};

// Fallback CRC-memcpy engine.
class FallbackCrcMemcpyEngine : public CrcMemcpyEngine {
 public:
  FallbackCrcMemcpyEngine() = default;
  FallbackCrcMemcpyEngine(const FallbackCrcMemcpyEngine&) = delete;
  FallbackCrcMemcpyEngine operator=(const FallbackCrcMemcpyEngine&) = delete;

  crc32c_t Compute(void* __restrict dst, const void* __restrict src,
                   std::size_t length, crc32c_t initial_crc) const override;
};

// CRC Non-Temporal-Memcpy engine.
class CrcNonTemporalMemcpyEngine : public CrcMemcpyEngine {
 public:
  CrcNonTemporalMemcpyEngine() = default;
  CrcNonTemporalMemcpyEngine(const CrcNonTemporalMemcpyEngine&) = delete;
  CrcNonTemporalMemcpyEngine operator=(const CrcNonTemporalMemcpyEngine&) =
      delete;

  crc32c_t Compute(void* __restrict dst, const void* __restrict src,
                   std::size_t length, crc32c_t initial_crc) const override;
};

// CRC Non-Temporal-Memcpy AVX engine.
class CrcNonTemporalMemcpyAVXEngine : public CrcMemcpyEngine {
 public:
  CrcNonTemporalMemcpyAVXEngine() = default;
  CrcNonTemporalMemcpyAVXEngine(const CrcNonTemporalMemcpyAVXEngine&) = delete;
  CrcNonTemporalMemcpyAVXEngine operator=(
      const CrcNonTemporalMemcpyAVXEngine&) = delete;

  crc32c_t Compute(void* __restrict dst, const void* __restrict src,
                   std::size_t length, crc32c_t initial_crc) const override;
};

// Copy source to destination and return the CRC32C of the data copied.  If an
// accelerated version is available, use the accelerated version, otherwise use
// the generic fallback version.
inline crc32c_t Crc32CAndCopy(void* __restrict dst, const void* __restrict src,
                              std::size_t length,
                              crc32c_t initial_crc = crc32c_t{0},
                              bool non_temporal = false) {
  return CrcMemcpy::CrcAndCopy(dst, src, length, initial_crc, non_temporal);
}

}  // namespace crc_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CRC_INTERNAL_CRC_MEMCPY_H_
