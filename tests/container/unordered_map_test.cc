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

#include <memory>
#include <unordered_map>

#include <tests/container/unordered_map_constructor_test.h>
#include <tests/container/unordered_map_lookup_test.h>
#include <tests/container/unordered_map_members_test.h>
#include <tests/container/unordered_map_modifiers_test.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace {

using MapTypes = ::testing::Types<
    std::unordered_map<int, int, StatefulTestingHash, StatefulTestingEqual,
                       Alloc<std::pair<const int, int>>>,
    std::unordered_map<std::string, std::string, StatefulTestingHash,
                       StatefulTestingEqual,
                       Alloc<std::pair<const std::string, std::string>>>>;

INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, ConstructorTest, MapTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, LookupTest, MapTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, MembersTest, MapTypes);
INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, ModifiersTest, MapTypes);

using UniquePtrMapTypes = ::testing::Types<std::unordered_map<
    int, std::unique_ptr<int>, StatefulTestingHash, StatefulTestingEqual,
    Alloc<std::pair<const int, std::unique_ptr<int>>>>>;

INSTANTIATE_TYPED_TEST_SUITE_P(UnorderedMap, UniquePtrModifiersTest,
                               UniquePtrMapTypes);

}  // namespace
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
