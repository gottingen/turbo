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

#include <tests/base/atomic_hook_test_helper.h>
#include <turbo/base/attributes.h>
#include <turbo/base/internal/atomic_hook.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace atomic_hook_internal {

TURBO_INTERNAL_ATOMIC_HOOK_ATTRIBUTES turbo::base_internal::AtomicHook<VoidF>
    func(DefaultFunc);
TURBO_CONST_INIT int default_func_calls = 0;
void DefaultFunc() { default_func_calls++; }
void RegisterFunc(VoidF f) { func.Store(f); }

}  // namespace atomic_hook_internal
TURBO_NAMESPACE_END
}  // namespace turbo
