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

#include <turbo/log/initialize.h>

#include <turbo/base/config.h>
#include <turbo/log/internal/globals.h>
#include <turbo/times/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace {
void InitializeLogImpl(turbo::TimeZone time_zone) {
  // This comes first since it is used by RAW_LOG.
  turbo::log_internal::SetTimeZone(time_zone);

  // Note that initialization is complete, so logs can now be sent to their
  // proper destinations rather than stderr.
  log_internal::SetInitialized();
}
}  // namespace

void initialize_log() { InitializeLogImpl(turbo::LocalTimeZone()); }

TURBO_NAMESPACE_END
}  // namespace turbo
