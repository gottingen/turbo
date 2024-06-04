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

#include <turbo/strings/internal/cord_rep_crc.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/crypto/internal/crc_cord_state.h>
#include <turbo/strings/internal/cord_internal.h>
#include <turbo/strings/internal/cord_rep_test_util.h>

namespace turbo {
    TURBO_NAMESPACE_BEGIN
    namespace cord_internal {
        namespace {

            using ::turbo::cordrep_testing::MakeFlat;
            using ::testing::Eq;
            using ::testing::IsNull;
            using ::testing::Ne;

#if !defined(NDEBUG) && GTEST_HAS_DEATH_TEST

            TEST(CordRepCrc, RemoveCrcWithNullptr) {
                EXPECT_DEATH(RemoveCrcNode(nullptr), "");
            }

#endif  // !NDEBUG && GTEST_HAS_DEATH_TEST

            turbo::crc_internal::CrcCordState MakeCrcCordState(uint32_t crc) {
                crc_internal::CrcCordState state;
                state.mutable_rep()->prefix_crc.push_back(
                        crc_internal::CrcCordState::PrefixCrc(42, CRC32C{crc}));
                return state;
            }

            TEST(CordRepCrc, NewDestroy) {
                CordRep *rep = cordrep_testing::MakeFlat("Hello world");
                CordRepCrc *crc = CordRepCrc::New(rep, MakeCrcCordState(12345));
                EXPECT_TRUE(crc->refcount.IsOne());
                EXPECT_THAT(crc->child, Eq(rep));
                EXPECT_THAT(crc->crc_cord_state.Checksum(), Eq(CRC32C{12345u}));
                EXPECT_TRUE(rep->refcount.IsOne());
                CordRepCrc::Destroy(crc);
            }

            TEST(CordRepCrc, NewExistingCrcNotShared) {
                CordRep *rep = cordrep_testing::MakeFlat("Hello world");
                CordRepCrc *crc = CordRepCrc::New(rep, MakeCrcCordState(12345));
                CordRepCrc *new_crc = CordRepCrc::New(crc, MakeCrcCordState(54321));
                EXPECT_THAT(new_crc, Eq(crc));
                EXPECT_TRUE(new_crc->refcount.IsOne());
                EXPECT_THAT(new_crc->child, Eq(rep));
                EXPECT_THAT(new_crc->crc_cord_state.Checksum(), Eq(CRC32C{54321u}));
                EXPECT_TRUE(rep->refcount.IsOne());
                CordRepCrc::Destroy(new_crc);
            }

            TEST(CordRepCrc, NewExistingCrcShared) {
                CordRep *rep = cordrep_testing::MakeFlat("Hello world");
                CordRepCrc *crc = CordRepCrc::New(rep, MakeCrcCordState(12345));
                CordRep::Ref(crc);
                CordRepCrc *new_crc = CordRepCrc::New(crc, MakeCrcCordState(54321));

                EXPECT_THAT(new_crc, Ne(crc));
                EXPECT_TRUE(new_crc->refcount.IsOne());
                EXPECT_TRUE(crc->refcount.IsOne());
                EXPECT_FALSE(rep->refcount.IsOne());
                EXPECT_THAT(crc->child, Eq(rep));
                EXPECT_THAT(new_crc->child, Eq(rep));
                EXPECT_THAT(crc->crc_cord_state.Checksum(), Eq(CRC32C{12345u}));
                EXPECT_THAT(new_crc->crc_cord_state.Checksum(), Eq(CRC32C{54321u}));

                CordRep::Unref(crc);
                CordRep::Unref(new_crc);
            }

            TEST(CordRepCrc, NewEmpty) {
                CordRepCrc *crc = CordRepCrc::New(nullptr, MakeCrcCordState(12345));
                EXPECT_TRUE(crc->refcount.IsOne());
                EXPECT_THAT(crc->child, IsNull());
                EXPECT_THAT(crc->length, Eq(0u));
                EXPECT_THAT(crc->crc_cord_state.Checksum(), Eq(CRC32C{12345u}));
                EXPECT_TRUE(crc->refcount.IsOne());
                CordRepCrc::Destroy(crc);
            }

            TEST(CordRepCrc, RemoveCrcNotCrc) {
                CordRep *rep = cordrep_testing::MakeFlat("Hello world");
                CordRep *nocrc = RemoveCrcNode(rep);
                EXPECT_THAT(nocrc, Eq(rep));
                CordRep::Unref(nocrc);
            }

            TEST(CordRepCrc, RemoveCrcNotShared) {
                CordRep *rep = cordrep_testing::MakeFlat("Hello world");
                CordRepCrc *crc = CordRepCrc::New(rep, MakeCrcCordState(12345));
                CordRep *nocrc = RemoveCrcNode(crc);
                EXPECT_THAT(nocrc, Eq(rep));
                EXPECT_TRUE(rep->refcount.IsOne());
                CordRep::Unref(nocrc);
            }

            TEST(CordRepCrc, RemoveCrcShared) {
                CordRep *rep = cordrep_testing::MakeFlat("Hello world");
                CordRepCrc *crc = CordRepCrc::New(rep, MakeCrcCordState(12345));
                CordRep::Ref(crc);
                CordRep *nocrc = RemoveCrcNode(crc);
                EXPECT_THAT(nocrc, Eq(rep));
                EXPECT_FALSE(rep->refcount.IsOne());
                CordRep::Unref(nocrc);
                CordRep::Unref(crc);
            }

        }  // namespace
    }  // namespace cord_internal
    TURBO_NAMESPACE_END
}  // namespace turbo
