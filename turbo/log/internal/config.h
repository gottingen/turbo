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
//
// -----------------------------------------------------------------------------
// File: log/internal/config.h
// -----------------------------------------------------------------------------
//

#ifndef TURBO_LOG_INTERNAL_CONFIG_H_
#define TURBO_LOG_INTERNAL_CONFIG_H_

#include <turbo/base/config.h>

#ifdef _WIN32
#include <cstdint>
#else
#include <sys/types.h>
#endif

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

#ifdef _WIN32
using Tid = uint32_t;
#else
using Tid = pid_t;
#endif

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_CONFIG_H_
