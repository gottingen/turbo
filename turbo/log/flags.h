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
// File: log/flags.h
// -----------------------------------------------------------------------------
//

#pragma once

// The Turbo Logging library supports the following command line flags to
// configure logging behavior at runtime:
//
// --stderrthreshold=<value>
// Log messages at or above this threshold level are copied to stderr.
//
// --minloglevel=<value>
// Messages logged at a lower level than this are discarded and don't actually
// get logged anywhere.
//
// --log_backtrace_at=<file:linenum>
// Emit a backtrace (stack trace) when logging at file:linenum.
//
// To use these commandline flags, the //turbo/log:flags library must be
// explicitly linked, and turbo::ParseCommandLine() must be called before the
// call to turbo::initialize_log().
//
// To configure the Log library programmatically, use the interfaces defined in
// turbo/log/globals.h.
