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

#ifndef TURBO_RANDOM_INTERNAL_MOCK_VALIDATORS_H_
#define TURBO_RANDOM_INTERNAL_MOCK_VALIDATORS_H_

#include <type_traits>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/random/internal/iostream_state_saver.h>
#include <turbo/random/internal/uniform_helper.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace random_internal {

template <typename NumType>
class UniformDistributionValidator {
 public:
  // Handle turbo::Uniform<NumType>(gen, turbo::IntervalTag, lo, hi).
  template <typename TagType>
  static void Validate(NumType x, TagType tag, NumType lo, NumType hi) {
    // For invalid ranges, turbo::Uniform() simply returns one of the bounds.
    if (x == lo && lo == hi) return;

    ValidateImpl(std::is_floating_point<NumType>{}, x, tag, lo, hi);
  }

  // Handle turbo::Uniform<NumType>(gen, lo, hi).
  static void Validate(NumType x, NumType lo, NumType hi) {
    Validate(x, IntervalClosedOpenTag(), lo, hi);
  }

  // Handle turbo::Uniform<NumType>(gen).
  static void Validate(NumType) {
    // turbo::Uniform<NumType>(gen) spans the entire range of `NumType`, so any
    // value is okay. This overload exists because the validation logic attempts
    // to call it anyway rather than adding extra SFINAE.
  }

 private:
  static turbo::string_view TagLbBound(IntervalClosedOpenTag) { return "["; }
  static turbo::string_view TagLbBound(IntervalOpenOpenTag) { return "("; }
  static turbo::string_view TagLbBound(IntervalClosedClosedTag) { return "["; }
  static turbo::string_view TagLbBound(IntervalOpenClosedTag) { return "("; }
  static turbo::string_view TagUbBound(IntervalClosedOpenTag) { return ")"; }
  static turbo::string_view TagUbBound(IntervalOpenOpenTag) { return ")"; }
  static turbo::string_view TagUbBound(IntervalClosedClosedTag) { return "]"; }
  static turbo::string_view TagUbBound(IntervalOpenClosedTag) { return "]"; }

  template <typename TagType>
  static void ValidateImpl(std::true_type /* is_floating_point */, NumType x,
                           TagType tag, NumType lo, NumType hi) {
    UniformDistributionWrapper<NumType> dist(tag, lo, hi);
    NumType lb = dist.a();
    NumType ub = dist.b();
    // uniform_real_distribution is always closed-open, so the upper bound is
    // always non-inclusive.
    TURBO_INTERNAL_CHECK(lb <= x && x < ub,
                        turbo::StrCat(x, " is not in ", TagLbBound(tag), lo,
                                     ", ", hi, TagUbBound(tag)));
  }

  template <typename TagType>
  static void ValidateImpl(std::false_type /* is_floating_point */, NumType x,
                           TagType tag, NumType lo, NumType hi) {
    using stream_type =
        typename random_internal::stream_format_type<NumType>::type;

    UniformDistributionWrapper<NumType> dist(tag, lo, hi);
    NumType lb = dist.a();
    NumType ub = dist.b();
    TURBO_INTERNAL_CHECK(
        lb <= x && x <= ub,
        turbo::StrCat(stream_type{x}, " is not in ", TagLbBound(tag),
                     stream_type{lo}, ", ", stream_type{hi}, TagUbBound(tag)));
  }
};

}  // namespace random_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_INTERNAL_MOCK_VALIDATORS_H_
