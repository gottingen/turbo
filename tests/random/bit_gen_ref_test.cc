//
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
//
#include <turbo/random/bit_gen_ref.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/internal/fast_type_id.h>
#include <turbo/random/internal/sequence_urbg.h>
#include <turbo/random/random.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

class ConstBitGen {
 public:
  // URBG interface
  using result_type = turbo::BitGen::result_type;

  static constexpr result_type(min)() { return (turbo::BitGen::min)(); }
  static constexpr result_type(max)() { return (turbo::BitGen::max)(); }
  result_type operator()() { return 1; }

  // InvokeMock method
  bool InvokeMock(base_internal::FastTypeIdType index, void*, void* result) {
    *static_cast<int*>(result) = 42;
    return true;
  }
};

namespace {

int FnTest(turbo::BitGenRef gen_ref) { return turbo::Uniform(gen_ref, 1, 7); }

template <typename T>
class BitGenRefTest : public testing::Test {};

using BitGenTypes =
    ::testing::Types<turbo::BitGen, turbo::InsecureBitGen, std::mt19937,
                     std::mt19937_64, std::minstd_rand>;
TYPED_TEST_SUITE(BitGenRefTest, BitGenTypes);

TYPED_TEST(BitGenRefTest, BasicTest) {
  TypeParam gen;
  auto x = FnTest(gen);
  EXPECT_NEAR(x, 4, 3);
}

TYPED_TEST(BitGenRefTest, Copyable) {
  TypeParam gen;
  turbo::BitGenRef gen_ref(gen);
  FnTest(gen_ref);  // Copy
}

TEST(BitGenRefTest, PassThroughEquivalence) {
  // sequence_urbg returns 64-bit results.
  turbo::random_internal::sequence_urbg urbg(
      {0x0003eb76f6f7f755ull, 0xFFCEA50FDB2F953Bull, 0xC332DDEFBE6C5AA5ull,
       0x6558218568AB9702ull, 0x2AEF7DAD5B6E2F84ull, 0x1521B62829076170ull,
       0xECDD4775619F1510ull, 0x13CCA830EB61BD96ull, 0x0334FE1EAA0363CFull,
       0xB5735C904C70A239ull, 0xD59E9E0BCBAADE14ull, 0xEECC86BC60622CA7ull});

  std::vector<uint64_t> output(12);

  {
    turbo::BitGenRef view(urbg);
    for (auto& v : output) {
      v = view();
    }
  }

  std::vector<uint64_t> expected(
      {0x0003eb76f6f7f755ull, 0xFFCEA50FDB2F953Bull, 0xC332DDEFBE6C5AA5ull,
       0x6558218568AB9702ull, 0x2AEF7DAD5B6E2F84ull, 0x1521B62829076170ull,
       0xECDD4775619F1510ull, 0x13CCA830EB61BD96ull, 0x0334FE1EAA0363CFull,
       0xB5735C904C70A239ull, 0xD59E9E0BCBAADE14ull, 0xEECC86BC60622CA7ull});

  EXPECT_THAT(output, testing::Eq(expected));
}

TEST(BitGenRefTest, MockingBitGenBaseOverrides) {
  ConstBitGen const_gen;
  EXPECT_EQ(FnTest(const_gen), 42);

  turbo::BitGenRef gen_ref(const_gen);
  EXPECT_EQ(FnTest(gen_ref), 42);  // Copy
}
}  // namespace
TURBO_NAMESPACE_END
}  // namespace turbo
