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

#include <tests/container/hash_generator_testing.h>

#include <deque>

#include <turbo/base/no_destructor.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace hash_internal {
namespace {

class RandomDeviceSeedSeq {
 public:
  using result_type = typename std::random_device::result_type;

  template <class Iterator>
  void generate(Iterator start, Iterator end) {
    while (start != end) {
      *start = gen_();
      ++start;
    }
  }

 private:
  std::random_device gen_;
};

}  // namespace

std::mt19937_64* GetSharedRng() {
  static turbo::NoDestructor<std::mt19937_64> rng([] {
    RandomDeviceSeedSeq seed_seq;
    return std::mt19937_64(seed_seq);
  }());
  return rng.get();
}

std::string Generator<std::string>::operator()() const {
  // NOLINTNEXTLINE(runtime/int)
  std::uniform_int_distribution<short> chars(0x20, 0x7E);
  std::string res;
  res.resize(32);
  std::generate(res.begin(), res.end(),
                [&]() { return chars(*GetSharedRng()); });
  return res;
}

turbo::string_view Generator<turbo::string_view>::operator()() const {
  static turbo::NoDestructor<std::deque<std::string>> arena;
  // NOLINTNEXTLINE(runtime/int)
  std::uniform_int_distribution<short> chars(0x20, 0x7E);
  arena->emplace_back();
  auto& res = arena->back();
  res.resize(32);
  std::generate(res.begin(), res.end(),
                [&]() { return chars(*GetSharedRng()); });
  return res;
}

}  // namespace hash_internal
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
