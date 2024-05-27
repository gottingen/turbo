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
#ifndef TURBO_STATUS_STATUS_PAYLOAD_PRINTER_H_
#define TURBO_STATUS_STATUS_PAYLOAD_PRINTER_H_

#include <string>

#include <turbo/base/nullability.h>
#include <turbo/strings/cord.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/optional.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace status_internal {

// By default, `Status::ToString` and `operator<<(Status)` print a payload by
// dumping the type URL and the raw bytes. To help debugging, we provide an
// extension point, which is a global printer function that can be set by users
// to specify how to print payloads. The function takes the type URL and the
// payload as input, and should return a valid human-readable string on success
// or `turbo::nullopt` on failure (in which case it falls back to the default
// approach of printing the raw bytes).
// NOTE: This is an internal API and the design is subject to change in the
// future in a non-backward-compatible way. Since it's only meant for debugging
// purpose, you should not rely on it in any critical logic.
using StatusPayloadPrinter = turbo::Nullable<turbo::optional<std::string> (*)(
    turbo::string_view, const turbo::Cord&)>;

// Sets the global payload printer. Only one printer should be set per process.
// If multiple printers are set, it's undefined which one will be used.
void SetStatusPayloadPrinter(StatusPayloadPrinter);

// Returns the global payload printer if previously set, otherwise `nullptr`.
StatusPayloadPrinter GetStatusPayloadPrinter();

}  // namespace status_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STATUS_STATUS_PAYLOAD_PRINTER_H_
