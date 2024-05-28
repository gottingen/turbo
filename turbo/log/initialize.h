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
// File: log/initialize.h
// -----------------------------------------------------------------------------
//
// This header declares the Turbo Log initialization routine initialize_log().

#pragma once

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// initialize_log()
//
// Initializes the Turbo logging library.
//
// Before this function is called, all log messages are directed only to stderr.
// After initialization is finished, log messages are directed to all registered
// `LogSink`s.
//
// It is an error to call this function twice.
//
// There is no corresponding function to shut down the logging library.
void initialize_log();

TURBO_NAMESPACE_END
}  // namespace turbo
