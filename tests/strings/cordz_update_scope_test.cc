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

#include <turbo/strings/internal/cordz_update_scope.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <tests/strings/cordz_test_helpers.h>
#include <turbo/strings/internal/cord_rep_flat.h>
#include <turbo/strings/internal/cordz_info.h>
#include <turbo/strings/internal/cordz_update_tracker.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

namespace {

// Used test values
auto constexpr kTrackCordMethod = CordzUpdateTracker::kConstructorString;

TEST(CordzUpdateScopeTest, ScopeNullptr) {
  CordzUpdateScope scope(nullptr, kTrackCordMethod);
}

TEST(CordzUpdateScopeTest, ScopeSampledCord) {
  TestCordData cord;
  CordzInfo::TrackCord(cord.data, kTrackCordMethod, 1);
  CordzUpdateScope scope(cord.data.cordz_info(), kTrackCordMethod);
  cord.data.cordz_info()->SetCordRep(nullptr);
}

}  // namespace
TURBO_NAMESPACE_END
}  // namespace cord_internal

}  // namespace turbo
