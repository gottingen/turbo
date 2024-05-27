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

#ifndef TURBO_RANDOM_EXPONENTIAL_DISTRIBUTION_H_
#define TURBO_RANDOM_EXPONENTIAL_DISTRIBUTION_H_

#include <cassert>
#include <cmath>
#include <istream>
#include <limits>
#include <type_traits>

#include <turbo/meta/type_traits.h>
#include <turbo/random/internal/fast_uniform_bits.h>
#include <turbo/random/internal/generate_real.h>
#include <turbo/random/internal/iostream_state_saver.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// turbo::exponential_distribution:
// Generates a number conforming to an exponential distribution and is
// equivalent to the standard [rand.dist.pois.exp] distribution.
template <typename RealType = double>
class exponential_distribution {
 public:
  using result_type = RealType;

  class param_type {
   public:
    using distribution_type = exponential_distribution;

    explicit param_type(result_type lambda = 1) : lambda_(lambda) {
      assert(lambda > 0);
      neg_inv_lambda_ = -result_type(1) / lambda_;
    }

    result_type lambda() const { return lambda_; }

    friend bool operator==(const param_type& a, const param_type& b) {
      return a.lambda_ == b.lambda_;
    }

    friend bool operator!=(const param_type& a, const param_type& b) {
      return !(a == b);
    }

   private:
    friend class exponential_distribution;

    result_type lambda_;
    result_type neg_inv_lambda_;

    static_assert(
        std::is_floating_point<RealType>::value,
        "Class-template turbo::exponential_distribution<> must be parameterized "
        "using a floating-point type.");
  };

  exponential_distribution() : exponential_distribution(1) {}

  explicit exponential_distribution(result_type lambda) : param_(lambda) {}

  explicit exponential_distribution(const param_type& p) : param_(p) {}

  void reset() {}

  // Generating functions
  template <typename URBG>
  result_type operator()(URBG& g) {  // NOLINT(runtime/references)
    return (*this)(g, param_);
  }

  template <typename URBG>
  result_type operator()(URBG& g,  // NOLINT(runtime/references)
                         const param_type& p);

  param_type param() const { return param_; }
  void param(const param_type& p) { param_ = p; }

  result_type(min)() const { return 0; }
  result_type(max)() const {
    return std::numeric_limits<result_type>::infinity();
  }

  result_type lambda() const { return param_.lambda(); }

  friend bool operator==(const exponential_distribution& a,
                         const exponential_distribution& b) {
    return a.param_ == b.param_;
  }
  friend bool operator!=(const exponential_distribution& a,
                         const exponential_distribution& b) {
    return a.param_ != b.param_;
  }

 private:
  param_type param_;
  random_internal::FastUniformBits<uint64_t> fast_u64_;
};

// --------------------------------------------------------------------------
// Implementation details follow
// --------------------------------------------------------------------------

template <typename RealType>
template <typename URBG>
typename exponential_distribution<RealType>::result_type
exponential_distribution<RealType>::operator()(
    URBG& g,  // NOLINT(runtime/references)
    const param_type& p) {
  using random_internal::GenerateNegativeTag;
  using random_internal::GenerateRealFromBits;
  using real_type =
      turbo::conditional_t<std::is_same<RealType, float>::value, float, double>;

  const result_type u = GenerateRealFromBits<real_type, GenerateNegativeTag,
                                             false>(fast_u64_(g));  // U(-1, 0)

  // log1p(-x) is mathematically equivalent to log(1 - x) but has more
  // accuracy for x near zero.
  return p.neg_inv_lambda_ * std::log1p(u);
}

template <typename CharT, typename Traits, typename RealType>
std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os,  // NOLINT(runtime/references)
    const exponential_distribution<RealType>& x) {
  auto saver = random_internal::make_ostream_state_saver(os);
  os.precision(random_internal::stream_precision_helper<RealType>::kPrecision);
  os << x.lambda();
  return os;
}

template <typename CharT, typename Traits, typename RealType>
std::basic_istream<CharT, Traits>& operator>>(
    std::basic_istream<CharT, Traits>& is,    // NOLINT(runtime/references)
    exponential_distribution<RealType>& x) {  // NOLINT(runtime/references)
  using result_type = typename exponential_distribution<RealType>::result_type;
  using param_type = typename exponential_distribution<RealType>::param_type;
  result_type lambda;

  auto saver = random_internal::make_istream_state_saver(is);
  lambda = random_internal::read_floating_point<result_type>(is);
  if (!is.fail()) {
    x.param(param_type(lambda));
  }
  return is;
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_EXPONENTIAL_DISTRIBUTION_H_
