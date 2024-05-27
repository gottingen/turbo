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

#include <tests/container/test_instance_tracker.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace test_internal {
int BaseCountedInstance::num_instances_ = 0;
int BaseCountedInstance::num_live_instances_ = 0;
int BaseCountedInstance::num_moves_ = 0;
int BaseCountedInstance::num_copies_ = 0;
int BaseCountedInstance::num_swaps_ = 0;
int BaseCountedInstance::num_comparisons_ = 0;

}  // namespace test_internal
TURBO_NAMESPACE_END
}  // namespace turbo
