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

#ifndef TURBO_CRC_INTERNAL_CPU_DETECT_H_
#define TURBO_CRC_INTERNAL_CPU_DETECT_H_

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace crc_internal {

// Enumeration of architectures that we have special-case tuning parameters for.
// This set may change over time.
enum class CpuType {
  kUnknown,
  kIntelHaswell,
  kAmdRome,
  kAmdNaples,
  kAmdMilan,
  kAmdGenoa,
  kAmdRyzenV3000,
  kIntelCascadelakeXeon,
  kIntelSkylakeXeon,
  kIntelBroadwell,
  kIntelSkylake,
  kIntelIvybridge,
  kIntelSandybridge,
  kIntelWestmere,
  kArmNeoverseN1,
  kArmNeoverseV1,
  kAmpereSiryn,
  kArmNeoverseN2,
  kArmNeoverseV2
};

// Returns the type of host CPU this code is running on.  Returns kUnknown if
// the host CPU is of unknown type, or if detection otherwise fails.
CpuType GetCpuType();

// Returns whether the host CPU supports the CPU features needed for our
// accelerated implementations. The CpuTypes enumerated above apart from
// kUnknown support the required features. On unknown CPUs, we can use
// this to see if it's safe to use hardware acceleration, though without any
// tuning.
bool SupportsArmCRC32PMULL();

}  // namespace crc_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_CRC_INTERNAL_CPU_DETECT_H_
