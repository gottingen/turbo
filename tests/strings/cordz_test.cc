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

#include <cstddef>
#include <cstring>
#include <ostream>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/strings/cord.h>
#include <turbo/strings/cord_buffer.h>
#include <tests/strings/cord_test_helpers.h>
#include <tests/strings/cordz_test_helpers.h>
#include <turbo/strings/internal/cord_internal.h>
#include <turbo/strings/internal/cordz_info.h>
#include <turbo/strings/internal/cordz_sample_token.h>
#include <turbo/strings/internal/cordz_statistics.h>
#include <turbo/strings/internal/cordz_update_tracker.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>

#ifdef TURBO_INTERNAL_CORDZ_ENABLED

using testing::Eq;
using testing::AnyOf;

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    using cord_internal::CordzInfo;
    using cord_internal::CordzSampleToken;
    using cord_internal::CordzStatistics;
    using cord_internal::CordzUpdateTracker;
    using Method = CordzUpdateTracker::MethodIdentifier;

    // Do not print cord contents, we only care about 'size' perhaps.
    // Note that this method must be inside the named namespace.
    inline void PrintTo(const Cord &cord, std::ostream *s) {
        if (s) *s << "Cord[" << cord.size() << "]";
    }

    namespace {

        auto constexpr kMaxInline = cord_internal::kMaxInline;

        // Returns a std::string_view value of the specified length
        // We do this to avoid 'consuming' large strings in Cord by default.
        std::string_view MakeString(size_t size) {
            thread_local std::string str;
            str = std::string(size, '.');
            return str;
        }

        std::string_view MakeString(TestCordSize size) {
            return MakeString(Length(size));
        }

        // Returns a cord with a sampled method of kAppendString.
        turbo::Cord MakeAppendStringCord(TestCordSize size) {
            CordzSamplingIntervalHelper always(1);
            turbo::Cord cord;
            cord.append(MakeString(size));
            return cord;
        }

        std::string TestParamToString(::testing::TestParamInfo<TestCordSize> size) {
            return turbo::str_cat("On", ToString(size.param), "Cord");
        }

        class CordzUpdateTest : public testing::TestWithParam<TestCordSize> {
        public:
            Cord &cord() { return cord_; }

            Method InitialOr(Method method) const {
                return (GetParam() > TestCordSize::kInlined) ? Method::kConstructorString
                                                             : method;
            }

        private:
            CordzSamplingIntervalHelper sample_every_{1};
            Cord cord_{MakeString(GetParam())};
        };

        template<typename T>
        std::string ParamToString(::testing::TestParamInfo<T> param) {
            return std::string(ToString(param.param));
        }

        INSTANTIATE_TEST_SUITE_P(WithParam, CordzUpdateTest,
                                 testing::Values(TestCordSize::kEmpty,
                                                 TestCordSize::kInlined,
                                                 TestCordSize::kLarge),
                                 TestParamToString);

        class CordzStringTest : public testing::TestWithParam<TestCordSize> {
        private:
            CordzSamplingIntervalHelper sample_every_{1};
        };

        INSTANTIATE_TEST_SUITE_P(WithParam, CordzStringTest,
                                 testing::Values(TestCordSize::kInlined,
                                                 TestCordSize::kStringSso1,
                                                 TestCordSize::kStringSso2,
                                                 TestCordSize::kSmall,
                                                 TestCordSize::kLarge),
                                 ParamToString<TestCordSize>);

        TEST(CordzTest, ConstructSmallArray) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord cord(MakeString(TestCordSize::kSmall));
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
        }

        TEST(CordzTest, ConstructLargeArray) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord cord(MakeString(TestCordSize::kLarge));
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
        }

        TEST_P(CordzStringTest, ConstructString) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord cord(std::string(Length(GetParam()), '.'));
            if (Length(GetParam()) > kMaxInline) {
                EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
            }
        }

        TEST(CordzTest, CopyConstructFromUnsampled) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord src = UnsampledCord(MakeString(TestCordSize::kLarge));
            Cord cord(src);
            EXPECT_THAT(GetCordzInfoForTesting(cord), Eq(nullptr));
        }

        TEST(CordzTest, CopyConstructFromSampled) {
            CordzSamplingIntervalHelper sample_never{99999};
            Cord src = MakeAppendStringCord(TestCordSize::kLarge);
            Cord cord(src);
            ASSERT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorCord));
            CordzStatistics stats = GetCordzInfoForTesting(cord)->GetCordzStatistics();
            EXPECT_THAT(stats.parent_method, Eq(Method::kAppendString));
            EXPECT_THAT(stats.update_tracker.Value(Method::kAppendString), Eq(1));
        }

        TEST(CordzTest, MoveConstruct) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord src(MakeString(TestCordSize::kLarge));
            Cord cord(std::move(src));
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
        }

        TEST_P(CordzUpdateTest, AssignUnsampledCord) {
            Cord src = UnsampledCord(MakeString(TestCordSize::kLarge));
            const CordzInfo *info = GetCordzInfoForTesting(cord());
            cord() = src;
            EXPECT_THAT(GetCordzInfoForTesting(cord()), Eq(nullptr));
            EXPECT_FALSE(CordzInfoIsListed(info));
        }

        TEST_P(CordzUpdateTest, AssignSampledCord) {
            Cord src = MakeAppendStringCord(TestCordSize::kLarge);
            cord() = src;
            ASSERT_THAT(cord(), HasValidCordzInfoOf(Method::kAssignCord));
            CordzStatistics stats = GetCordzInfoForTesting(cord())->GetCordzStatistics();
            EXPECT_THAT(stats.parent_method, Eq(Method::kAppendString));
            EXPECT_THAT(stats.update_tracker.Value(Method::kAppendString), Eq(1));
            EXPECT_THAT(stats.update_tracker.Value(Method::kConstructorString), Eq(0));
        }

        TEST(CordzUpdateTest, AssignSampledCordToInlined) {
            CordzSamplingIntervalHelper sample_never{99999};
            Cord cord;
            Cord src = MakeAppendStringCord(TestCordSize::kLarge);
            cord = src;
            ASSERT_THAT(cord, HasValidCordzInfoOf(Method::kAssignCord));
            CordzStatistics stats = GetCordzInfoForTesting(cord)->GetCordzStatistics();
            EXPECT_THAT(stats.parent_method, Eq(Method::kAppendString));
            EXPECT_THAT(stats.update_tracker.Value(Method::kAppendString), Eq(1));
            EXPECT_THAT(stats.update_tracker.Value(Method::kConstructorString), Eq(0));
        }

        TEST(CordzUpdateTest, AssignSampledCordToUnsampledCord) {
            CordzSamplingIntervalHelper sample_never{99999};
            Cord cord = UnsampledCord(MakeString(TestCordSize::kLarge));
            Cord src = MakeAppendStringCord(TestCordSize::kLarge);
            cord = src;
            ASSERT_THAT(cord, HasValidCordzInfoOf(Method::kAssignCord));
            CordzStatistics stats = GetCordzInfoForTesting(cord)->GetCordzStatistics();
            EXPECT_THAT(stats.parent_method, Eq(Method::kAppendString));
            EXPECT_THAT(stats.update_tracker.Value(Method::kAppendString), Eq(1));
            EXPECT_THAT(stats.update_tracker.Value(Method::kConstructorString), Eq(0));
        }

        TEST(CordzUpdateTest, AssignUnsampledCordToSampledCordWithoutSampling) {
            CordzSamplingIntervalHelper sample_never{99999};
            Cord cord = MakeAppendStringCord(TestCordSize::kLarge);
            const CordzInfo *info = GetCordzInfoForTesting(cord);
            Cord src = UnsampledCord(MakeString(TestCordSize::kLarge));
            cord = src;
            EXPECT_THAT(GetCordzInfoForTesting(cord), Eq(nullptr));
            EXPECT_FALSE(CordzInfoIsListed(info));
        }

        TEST(CordzUpdateTest, AssignUnsampledCordToSampledCordWithSampling) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord cord = MakeAppendStringCord(TestCordSize::kLarge);
            const CordzInfo *info = GetCordzInfoForTesting(cord);
            Cord src = UnsampledCord(MakeString(TestCordSize::kLarge));
            cord = src;
            EXPECT_THAT(GetCordzInfoForTesting(cord), Eq(nullptr));
            EXPECT_FALSE(CordzInfoIsListed(info));
        }

        TEST(CordzUpdateTest, AssignSampledCordToSampledCord) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord src = MakeAppendStringCord(TestCordSize::kLarge);
            Cord cord(MakeString(TestCordSize::kLarge));
            cord = src;
            ASSERT_THAT(cord, HasValidCordzInfoOf(Method::kAssignCord));
            CordzStatistics stats = GetCordzInfoForTesting(cord)->GetCordzStatistics();
            EXPECT_THAT(stats.parent_method, Eq(Method::kAppendString));
            EXPECT_THAT(stats.update_tracker.Value(Method::kAppendString), Eq(1));
            EXPECT_THAT(stats.update_tracker.Value(Method::kConstructorString), Eq(0));
        }

        TEST(CordzUpdateTest, AssignUnsampledCordToSampledCord) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord src = MakeAppendStringCord(TestCordSize::kLarge);
            Cord cord(MakeString(TestCordSize::kLarge));
            cord = src;
            ASSERT_THAT(cord, HasValidCordzInfoOf(Method::kAssignCord));
            CordzStatistics stats = GetCordzInfoForTesting(cord)->GetCordzStatistics();
            EXPECT_THAT(stats.parent_method, Eq(Method::kAppendString));
            EXPECT_THAT(stats.update_tracker.Value(Method::kAppendString), Eq(1));
            EXPECT_THAT(stats.update_tracker.Value(Method::kConstructorString), Eq(0));
        }

        TEST(CordzTest, AssignInlinedCordToSampledCord) {
            CordzSampleToken token;
            CordzSamplingIntervalHelper sample_every{1};
            Cord cord(MakeString(TestCordSize::kLarge));
            const CordzInfo *info = GetCordzInfoForTesting(cord);
            Cord src = UnsampledCord(MakeString(TestCordSize::kInlined));
            cord = src;
            EXPECT_THAT(GetCordzInfoForTesting(cord), Eq(nullptr));
            EXPECT_FALSE(CordzInfoIsListed(info));
        }

        TEST(CordzUpdateTest, MoveAssignCord) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord cord;
            Cord src(MakeString(TestCordSize::kLarge));
            cord = std::move(src);
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
        }

        TEST_P(CordzUpdateTest, AssignLargeArray) {
            cord() = MakeString(TestCordSize::kSmall);
            EXPECT_THAT(cord(), HasValidCordzInfoOf(Method::kAssignString));
        }

        TEST_P(CordzUpdateTest, AssignSmallArray) {
            cord() = MakeString(TestCordSize::kSmall);
            EXPECT_THAT(cord(), HasValidCordzInfoOf(Method::kAssignString));
        }

        TEST_P(CordzUpdateTest, AssignInlinedArray) {
            cord() = MakeString(TestCordSize::kInlined);
            EXPECT_THAT(GetCordzInfoForTesting(cord()), Eq(nullptr));
        }

        TEST_P(CordzStringTest, AssignStringToInlined) {
            Cord cord;
            cord = std::string(Length(GetParam()), '.');
            if (Length(GetParam()) > kMaxInline) {
                EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kAssignString));
            }
        }

        TEST_P(CordzStringTest, AssignStringToCord) {
            Cord cord(MakeString(TestCordSize::kLarge));
            cord = std::string(Length(GetParam()), '.');
            if (Length(GetParam()) > kMaxInline) {
                EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
                EXPECT_THAT(cord, CordzMethodCountEq(Method::kAssignString, 1));
            }
        }

        TEST_P(CordzUpdateTest, AssignInlinedString) {
            cord() = std::string(Length(TestCordSize::kInlined), '.');
            EXPECT_THAT(GetCordzInfoForTesting(cord()), Eq(nullptr));
        }

        TEST_P(CordzUpdateTest, AppendCord) {
            Cord src = UnsampledCord(MakeString(TestCordSize::kLarge));
            cord().append(src);
            EXPECT_THAT(cord(), HasValidCordzInfoOf(InitialOr(Method::kAppendCord)));
        }

        TEST_P(CordzUpdateTest, MoveAppendCord) {
            cord().append(UnsampledCord(MakeString(TestCordSize::kLarge)));
            EXPECT_THAT(cord(), HasValidCordzInfoOf(InitialOr(Method::kAppendCord)));
        }

        TEST_P(CordzUpdateTest, AppendSmallArray) {
            cord().append(MakeString(TestCordSize::kSmall));
            EXPECT_THAT(cord(), HasValidCordzInfoOf(InitialOr(Method::kAppendString)));
        }

        TEST_P(CordzUpdateTest, AppendLargeArray) {
            cord().append(MakeString(TestCordSize::kLarge));
            EXPECT_THAT(cord(), HasValidCordzInfoOf(InitialOr(Method::kAppendString)));
        }

        TEST_P(CordzStringTest, AppendStringToEmpty) {
            Cord cord;
            cord.append(std::string(Length(GetParam()), '.'));
            if (Length(GetParam()) > kMaxInline) {
                EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kAppendString));
            }
        }

        TEST_P(CordzStringTest, AppendStringToInlined) {
            Cord cord(MakeString(TestCordSize::kInlined));
            cord.append(std::string(Length(GetParam()), '.'));
            if (Length(TestCordSize::kInlined) + Length(GetParam()) > kMaxInline) {
                EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kAppendString));
            }
        }

        TEST_P(CordzStringTest, AppendStringToCord) {
            Cord cord(MakeString(TestCordSize::kLarge));
            cord.append(std::string(Length(GetParam()), '.'));
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
            EXPECT_THAT(cord, CordzMethodCountEq(Method::kAppendString, 1));
        }

        TEST(CordzTest, make_cord_from_external) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord cord = make_cord_from_external("Hello world", [](std::string_view) {});
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kmake_cord_from_external));
        }

        TEST(CordzTest, MakeCordFromEmptyExternal) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord cord = make_cord_from_external({}, [](std::string_view) {});
            EXPECT_THAT(GetCordzInfoForTesting(cord), Eq(nullptr));
        }

        TEST_P(CordzUpdateTest, PrependCord) {
            Cord src = UnsampledCord(MakeString(TestCordSize::kLarge));
            cord().prepend(src);
            EXPECT_THAT(cord(), HasValidCordzInfoOf(InitialOr(Method::kPrependCord)));
        }

        TEST_P(CordzUpdateTest, PrependSmallArray) {
            cord().prepend(MakeString(TestCordSize::kSmall));
            EXPECT_THAT(cord(), HasValidCordzInfoOf(InitialOr(Method::kPrependString)));
        }

        TEST_P(CordzUpdateTest, PrependLargeArray) {
            cord().prepend(MakeString(TestCordSize::kLarge));
            EXPECT_THAT(cord(), HasValidCordzInfoOf(InitialOr(Method::kPrependString)));
        }

        TEST_P(CordzStringTest, PrependStringToEmpty) {
            Cord cord;
            cord.prepend(std::string(Length(GetParam()), '.'));
            if (Length(GetParam()) > kMaxInline) {
                EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kPrependString));
            }
        }

        TEST_P(CordzStringTest, PrependStringToInlined) {
            Cord cord(MakeString(TestCordSize::kInlined));
            cord.prepend(std::string(Length(GetParam()), '.'));
            if (Length(TestCordSize::kInlined) + Length(GetParam()) > kMaxInline) {
                EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kPrependString));
            }
        }

        TEST_P(CordzStringTest, PrependStringToCord) {
            Cord cord(MakeString(TestCordSize::kLarge));
            cord.prepend(std::string(Length(GetParam()), '.'));
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
            EXPECT_THAT(cord, CordzMethodCountEq(Method::kPrependString, 1));
        }

        TEST(CordzTest, remove_prefix) {
            CordzSamplingIntervalHelper sample_every(1);
            Cord cord(MakeString(TestCordSize::kLarge));

            // Half the cord
            cord.remove_prefix(cord.size() / 2);
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
            EXPECT_THAT(cord, CordzMethodCountEq(Method::kRemovePrefix, 1));

            // TODO(mvels): remove_prefix does not reset to inlined, except if empty?
            cord.remove_prefix(cord.size() - kMaxInline);
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
            EXPECT_THAT(cord, CordzMethodCountEq(Method::kRemovePrefix, 2));

            cord.remove_prefix(cord.size());
            EXPECT_THAT(GetCordzInfoForTesting(cord), Eq(nullptr));
        }

        TEST(CordzTest, remove_suffix) {
            CordzSamplingIntervalHelper sample_every(1);
            Cord cord(MakeString(TestCordSize::kLarge));

            // Half the cord
            cord.remove_suffix(cord.size() / 2);
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
            EXPECT_THAT(cord, CordzMethodCountEq(Method::kRemoveSuffix, 1));

            // TODO(mvels): remove_suffix does not reset to inlined, except if empty?
            cord.remove_suffix(cord.size() - kMaxInline);
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kConstructorString));
            EXPECT_THAT(cord, CordzMethodCountEq(Method::kRemoveSuffix, 2));

            cord.remove_suffix(cord.size());
            EXPECT_THAT(GetCordzInfoForTesting(cord), Eq(nullptr));
        }

        TEST(CordzTest, SubCordFromUnsampledCord) {
            CordzSamplingIntervalHelper sample_every{1};
            Cord src = UnsampledCord(MakeString(TestCordSize::kLarge));
            Cord cord = src.subcord(10, src.size() / 2);
            EXPECT_THAT(GetCordzInfoForTesting(cord), Eq(nullptr));
        }

        TEST(CordzTest, SubCordFromSampledCord) {
            CordzSamplingIntervalHelper sample_never{99999};
            Cord src = MakeAppendStringCord(TestCordSize::kLarge);
            Cord cord = src.subcord(10, src.size() / 2);
            ASSERT_THAT(cord, HasValidCordzInfoOf(Method::kSubCord));
            CordzStatistics stats = GetCordzInfoForTesting(cord)->GetCordzStatistics();
            EXPECT_THAT(stats.parent_method, Eq(Method::kAppendString));
            EXPECT_THAT(stats.update_tracker.Value(Method::kAppendString), Eq(1));
        }

        TEST(CordzTest, SmallSubCord) {
            CordzSamplingIntervalHelper sample_never{99999};
            Cord src = MakeAppendStringCord(TestCordSize::kLarge);
            Cord cord = src.subcord(10, kMaxInline + 1);
            EXPECT_THAT(cord, HasValidCordzInfoOf(Method::kSubCord));
        }

    }  // namespace

    TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_INTERNAL_CORDZ_ENABLED
