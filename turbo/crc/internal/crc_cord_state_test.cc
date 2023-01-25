// Copyright 2022 The Turbo Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/crc/internal/crc_cord_state.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

#include "gtest/gtest.h"
#include "turbo/crc/crc32c.h"

namespace {

TEST(CrcCordState, Default) {
  turbo::crc_internal::CrcCordState state;
  EXPECT_TRUE(state.IsNormalized());
  EXPECT_EQ(state.Checksum(), turbo::crc32c_t{0});
  state.Normalize();
  EXPECT_EQ(state.Checksum(), turbo::crc32c_t{0});
}

TEST(CrcCordState, Normalize) {
  turbo::crc_internal::CrcCordState state;
  auto* rep = state.mutable_rep();
  rep->prefix_crc.push_back(
      turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::crc32c_t{1000}));
  rep->prefix_crc.push_back(
      turbo::crc_internal::CrcCordState::PrefixCrc(2000, turbo::crc32c_t{2000}));
  rep->removed_prefix =
      turbo::crc_internal::CrcCordState::PrefixCrc(500, turbo::crc32c_t{500});

  // The removed_prefix means state is not normalized.
  EXPECT_FALSE(state.IsNormalized());

  turbo::crc32c_t crc = state.Checksum();
  state.Normalize();
  EXPECT_TRUE(state.IsNormalized());

  // The checksum should not change as a result of calling Normalize().
  EXPECT_EQ(state.Checksum(), crc);
  EXPECT_EQ(rep->removed_prefix.length, 0);
}

TEST(CrcCordState, Copy) {
  turbo::crc_internal::CrcCordState state;
  auto* rep = state.mutable_rep();
  rep->prefix_crc.push_back(
      turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::crc32c_t{1000}));

  turbo::crc_internal::CrcCordState copy = state;

  EXPECT_EQ(state.Checksum(), turbo::crc32c_t{1000});
  EXPECT_EQ(copy.Checksum(), turbo::crc32c_t{1000});
}

TEST(CrcCordState, UnsharedSelfCopy) {
  turbo::crc_internal::CrcCordState state;
  auto* rep = state.mutable_rep();
  rep->prefix_crc.push_back(
      turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::crc32c_t{1000}));

  const turbo::crc_internal::CrcCordState& ref = state;
  state = ref;

  EXPECT_EQ(state.Checksum(), turbo::crc32c_t{1000});
}

TEST(CrcCordState, Move) {
  turbo::crc_internal::CrcCordState state;
  auto* rep = state.mutable_rep();
  rep->prefix_crc.push_back(
      turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::crc32c_t{1000}));

  turbo::crc_internal::CrcCordState moved = std::move(state);
  EXPECT_EQ(moved.Checksum(), turbo::crc32c_t{1000});
}

TEST(CrcCordState, UnsharedSelfMove) {
  turbo::crc_internal::CrcCordState state;
  auto* rep = state.mutable_rep();
  rep->prefix_crc.push_back(
      turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::crc32c_t{1000}));

  turbo::crc_internal::CrcCordState& ref = state;
  state = std::move(ref);

  EXPECT_EQ(state.Checksum(), turbo::crc32c_t{1000});
}

TEST(CrcCordState, PoisonDefault) {
  turbo::crc_internal::CrcCordState state;
  state.Poison();
  EXPECT_NE(state.Checksum(), turbo::crc32c_t{0});
}

TEST(CrcCordState, PoisonData) {
  turbo::crc_internal::CrcCordState state;
  auto* rep = state.mutable_rep();
  rep->prefix_crc.push_back(
      turbo::crc_internal::CrcCordState::PrefixCrc(1000, turbo::crc32c_t{1000}));
  rep->prefix_crc.push_back(
      turbo::crc_internal::CrcCordState::PrefixCrc(2000, turbo::crc32c_t{2000}));
  rep->removed_prefix =
      turbo::crc_internal::CrcCordState::PrefixCrc(500, turbo::crc32c_t{500});

  turbo::crc32c_t crc = state.Checksum();
  state.Poison();
  EXPECT_NE(state.Checksum(), crc);
}

}  // namespace
