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

#ifndef TURBO_RANDOM_INTERNAL_SEQUENCE_URBG_H_
#define TURBO_RANDOM_INTERNAL_SEQUENCE_URBG_H_

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>
#include <vector>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

// `sequence_urbg` is a simple random number generator which meets the
// requirements of [rand.req.urbg], and is solely for testing turbo
// distributions.
class sequence_urbg {
 public:
  using result_type = uint64_t;

  static constexpr result_type(min)() {
    return (std::numeric_limits<result_type>::min)();
  }
  static constexpr result_type(max)() {
    return (std::numeric_limits<result_type>::max)();
  }

  sequence_urbg(std::initializer_list<result_type> data) : i_(0), data_(data) {}
  void reset() { i_ = 0; }

  result_type operator()() { return data_[i_++ % data_.size()]; }

  size_t invocations() const { return i_; }

 private:
  size_t i_;
  std::vector<result_type> data_;
};

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_SEQUENCE_URBG_H_
