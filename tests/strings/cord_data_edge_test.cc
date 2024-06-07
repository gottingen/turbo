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

#include <turbo/strings/internal/cord_data_edge.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/strings/internal/cord_internal.h>
#include <turbo/strings/internal/cord_rep_test_util.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {
namespace {

using ::turbo::cordrep_testing::MakeExternal;
using ::turbo::cordrep_testing::MakeFlat;
using ::turbo::cordrep_testing::MakeSubstring;

TEST(CordDataEdgeTest, IsDataEdgeOnFlat) {
  CordRep* rep = MakeFlat("Lorem ipsum dolor sit amet, consectetur ...");
  EXPECT_TRUE(IsDataEdge(rep));
  CordRep::Unref(rep);
}

TEST(CordDataEdgeTest, IsDataEdgeOnExternal) {
  CordRep* rep = MakeExternal("Lorem ipsum dolor sit amet, consectetur ...");
  EXPECT_TRUE(IsDataEdge(rep));
  CordRep::Unref(rep);
}

TEST(CordDataEdgeTest, IsDataEdgeOnSubstringOfFlat) {
  CordRep* rep = MakeFlat("Lorem ipsum dolor sit amet, consectetur ...");
  CordRep* substr = MakeSubstring(1, 20, rep);
  EXPECT_TRUE(IsDataEdge(substr));
  CordRep::Unref(substr);
}

TEST(CordDataEdgeTest, IsDataEdgeOnSubstringOfExternal) {
  CordRep* rep = MakeExternal("Lorem ipsum dolor sit amet, consectetur ...");
  CordRep* substr = MakeSubstring(1, 20, rep);
  EXPECT_TRUE(IsDataEdge(substr));
  CordRep::Unref(substr);
}

TEST(CordDataEdgeTest, IsDataEdgeOnBtree) {
  CordRep* rep = MakeFlat("Lorem ipsum dolor sit amet, consectetur ...");
  CordRepBtree* tree = CordRepBtree::New(rep);
  EXPECT_FALSE(IsDataEdge(tree));
  CordRep::Unref(tree);
}

TEST(CordDataEdgeTest, IsDataEdgeOnBadSubstr) {
  CordRep* rep = MakeFlat("Lorem ipsum dolor sit amet, consectetur ...");
  CordRep* substr = MakeSubstring(1, 18, MakeSubstring(1, 20, rep));
  EXPECT_FALSE(IsDataEdge(substr));
  CordRep::Unref(substr);
}

TEST(CordDataEdgeTest, EdgeDataOnFlat) {
  std::string_view value = "Lorem ipsum dolor sit amet, consectetur ...";
  CordRep* rep = MakeFlat(value);
  EXPECT_EQ(EdgeData(rep), value);
  CordRep::Unref(rep);
}

TEST(CordDataEdgeTest, EdgeDataOnExternal) {
  std::string_view value = "Lorem ipsum dolor sit amet, consectetur ...";
  CordRep* rep = MakeExternal(value);
  EXPECT_EQ(EdgeData(rep), value);
  CordRep::Unref(rep);
}

TEST(CordDataEdgeTest, EdgeDataOnSubstringOfFlat) {
  std::string_view value = "Lorem ipsum dolor sit amet, consectetur ...";
  CordRep* rep = MakeFlat(value);
  CordRep* substr = MakeSubstring(1, 20, rep);
  EXPECT_EQ(EdgeData(substr), value.substr(1, 20));
  CordRep::Unref(substr);
}

TEST(CordDataEdgeTest, EdgeDataOnSubstringOfExternal) {
  std::string_view value = "Lorem ipsum dolor sit amet, consectetur ...";
  CordRep* rep = MakeExternal(value);
  CordRep* substr = MakeSubstring(1, 20, rep);
  EXPECT_EQ(EdgeData(substr), value.substr(1, 20));
  CordRep::Unref(substr);
}

#if defined(GTEST_HAS_DEATH_TEST) && !defined(NDEBUG)

TEST(CordDataEdgeTest, IsDataEdgeOnNullPtr) {
  EXPECT_DEATH(IsDataEdge(nullptr), ".*");
}

TEST(CordDataEdgeTest, EdgeDataOnNullPtr) {
  EXPECT_DEATH(EdgeData(nullptr), ".*");
}

TEST(CordDataEdgeTest, EdgeDataOnBtree) {
  CordRep* rep = MakeFlat("Lorem ipsum dolor sit amet, consectetur ...");
  CordRepBtree* tree = CordRepBtree::New(rep);
  EXPECT_DEATH(EdgeData(tree), ".*");
  CordRep::Unref(tree);
}

TEST(CordDataEdgeTest, EdgeDataOnBadSubstr) {
  CordRep* rep = MakeFlat("Lorem ipsum dolor sit amet, consectetur ...");
  CordRep* substr = MakeSubstring(1, 18, MakeSubstring(1, 20, rep));
  EXPECT_DEATH(EdgeData(substr), ".*");
  CordRep::Unref(substr);
}

#endif  // GTEST_HAS_DEATH_TEST && !NDEBUG

}  // namespace
}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
