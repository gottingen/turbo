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
#include <turbo/status/status_payload_printer.h>

#include <turbo/base/config.h>
#include <turbo/base/internal/atomic_hook.h>

namespace turbo::status_internal {

    TURBO_INTERNAL_ATOMIC_HOOK_ATTRIBUTES
    static turbo::base_internal::AtomicHook<StatusPayloadPrinter> storage;

    void SetStatusPayloadPrinter(StatusPayloadPrinter printer) {
        storage.Store(printer);
    }

    StatusPayloadPrinter GetStatusPayloadPrinter() {
        return storage.Load();
    }

}  // namespace turbo::status_internal
