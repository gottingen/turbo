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

#include <unordered_set>

#include <tests/container/unordered_set_constructor_test.h>
#include <tests/container/unordered_set_lookup_test.h>
#include <tests/container/unordered_set_members_test.h>
#include <tests/container/unordered_set_modifiers_test.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace {

using SetTypes = ::testing::Types<
    std::unordered_set<int, StatefulTestingHash, StatefulTestingEqual,
                       Alloc<int>>,
    std::unordered_set<std::string, StatefulTestingHash, StatefulTestingEqual,
                       Alloc<std::string>>>;

INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedSet, ConstructorTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedSet, LookupTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedSet, MembersTest, SetTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedSet, ModifiersTest, SetTypes);

}  // namespace
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
