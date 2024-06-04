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

#include <turbo/crypto/internal/crc_cord_state.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include <turbo/crypto/crc32c.h>

namespace {

    TEST(CrcCordState, Default) {
        turbo::crc_internal::CrcCordState state;
        EXPECT_TRUE(state.IsNormalized());
        EXPECT_EQ(state.Checksum(), turbo::CRC32C{0});
        state.Normalize();
        EXPECT_EQ(state.Checksum(), turbo::CRC32C{0});
    }

    TEST(CrcCordState, Normalize) {
        turbo::crc_internal::CrcCordState state;
        auto *rep = state.mutable_rep();
        rep->prefix_crc.push_back(
                turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::CRC32C{1000}));
        rep->prefix_crc.push_back(
                turbo::crc_internal::CrcCordState::PrefixCrc(2000, turbo::CRC32C{2000}));
        rep->removed_prefix =
                turbo::crc_internal::CrcCordState::PrefixCrc(500, turbo::CRC32C{500});

        // The removed_prefix means state is not normalized.
        EXPECT_FALSE(state.IsNormalized());

        turbo::CRC32C crc = state.Checksum();
        state.Normalize();
        EXPECT_TRUE(state.IsNormalized());

        // The checksum should not change as a result of calling Normalize().
        EXPECT_EQ(state.Checksum(), crc);
        EXPECT_EQ(rep->removed_prefix.length, 0);
    }

    TEST(CrcCordState, Copy) {
        turbo::crc_internal::CrcCordState state;
        auto *rep = state.mutable_rep();
        rep->prefix_crc.push_back(
                turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::CRC32C{1000}));

        turbo::crc_internal::CrcCordState copy = state;

        EXPECT_EQ(state.Checksum(), turbo::CRC32C{1000});
        EXPECT_EQ(copy.Checksum(), turbo::CRC32C{1000});
    }

    TEST(CrcCordState, UnsharedSelfCopy) {
        turbo::crc_internal::CrcCordState state;
        auto *rep = state.mutable_rep();
        rep->prefix_crc.push_back(
                turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::CRC32C{1000}));

        const turbo::crc_internal::CrcCordState &ref = state;
        state = ref;

        EXPECT_EQ(state.Checksum(), turbo::CRC32C{1000});
    }

    TEST(CrcCordState, Move) {
        turbo::crc_internal::CrcCordState state;
        auto *rep = state.mutable_rep();
        rep->prefix_crc.push_back(
                turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::CRC32C{1000}));

        turbo::crc_internal::CrcCordState moved = std::move(state);
        EXPECT_EQ(moved.Checksum(), turbo::CRC32C{1000});
    }

    TEST(CrcCordState, UnsharedSelfMove) {
        turbo::crc_internal::CrcCordState state;
        auto *rep = state.mutable_rep();
        rep->prefix_crc.push_back(
                turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::CRC32C{1000}));

        turbo::crc_internal::CrcCordState &ref = state;
        state = std::move(ref);

        EXPECT_EQ(state.Checksum(), turbo::CRC32C{1000});
    }

    TEST(CrcCordState, PoisonDefault) {
        turbo::crc_internal::CrcCordState state;
        state.Poison();
        EXPECT_NE(state.Checksum(), turbo::CRC32C{0});
    }

    TEST(CrcCordState, PoisonData) {
        turbo::crc_internal::CrcCordState state;
        auto *rep = state.mutable_rep();
        rep->prefix_crc.push_back(
                turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::CRC32C{1000}));
        rep->prefix_crc.push_back(
                turbo::crc_internal::CrcCordState::PrefixCrc(2000, turbo::CRC32C{2000}));
        rep->removed_prefix =
                turbo::crc_internal::CrcCordState::PrefixCrc(500, turbo::CRC32C{500});

        turbo::CRC32C crc = state.Checksum();
        state.Poison();
        EXPECT_NE(state.Checksum(), crc);
    }

}  // namespace
