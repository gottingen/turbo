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
// -----------------------------------------------------------------------------
// File: distributions.h
// -----------------------------------------------------------------------------
//
// This header defines functions representing distributions, which you use in
// combination with an Turbo random bit generator to produce random values
// according to the rules of that distribution.
//
// The Turbo random library defines the following distributions within this
// file:
//
//   * `turbo::Uniform` for uniform (constant) distributions having constant
//     probability
//   * `turbo::Bernoulli` for discrete distributions having exactly two outcomes
//   * `turbo::Beta` for continuous distributions parameterized through two
//     free parameters
//   * `turbo::Exponential` for discrete distributions of events occurring
//     continuously and independently at a constant average rate
//   * `turbo::Gaussian` (also known as "normal distributions") for continuous
//     distributions using an associated quadratic function
//   * `turbo::LogUniform` for discrete distributions where the log to the given
//     base of all values is uniform
//   * `turbo::Poisson` for discrete probability distributions that express the
//     probability of a given number of events occurring within a fixed interval
//   * `turbo::Zipf` for discrete probability distributions commonly used for
//     modelling of rare events
//
// Prefer use of these distribution function classes over manual construction of
// your own distribution classes, as it allows library maintainers greater
// flexibility to change the underlying implementation in the future.

#ifndef TURBO_RANDOM_DISTRIBUTIONS_H_
#define TURBO_RANDOM_DISTRIBUTIONS_H_

#include <limits>
#include <type_traits>

#include <turbo/base/config.h>
#include <turbo/base/internal/inline_variable.h>
#include <turbo/meta/type_traits.h>
#include <turbo/random/bernoulli_distribution.h>
#include <turbo/random/beta_distribution.h>
#include <turbo/random/exponential_distribution.h>
#include <turbo/random/gaussian_distribution.h>
#include <turbo/random/internal/distribution_caller.h>  // IWYU pragma: export
#include <turbo/random/internal/traits.h>
#include <turbo/random/internal/uniform_helper.h>  // IWYU pragma: export
#include <turbo/random/log_uniform_int_distribution.h>
#include <turbo/random/poisson_distribution.h>
#include <turbo/random/uniform_int_distribution.h>  // IWYU pragma: export
#include <turbo/random/uniform_real_distribution.h>  // IWYU pragma: export
#include <turbo/random/zipf_distribution.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

TURBO_INTERNAL_INLINE_CONSTEXPR(IntervalClosedClosedTag, IntervalClosedClosed,
                               {});
TURBO_INTERNAL_INLINE_CONSTEXPR(IntervalClosedClosedTag, IntervalClosed, {});
TURBO_INTERNAL_INLINE_CONSTEXPR(IntervalClosedOpenTag, IntervalClosedOpen, {});
TURBO_INTERNAL_INLINE_CONSTEXPR(IntervalOpenOpenTag, IntervalOpenOpen, {});
TURBO_INTERNAL_INLINE_CONSTEXPR(IntervalOpenOpenTag, IntervalOpen, {});
TURBO_INTERNAL_INLINE_CONSTEXPR(IntervalOpenClosedTag, IntervalOpenClosed, {});

// -----------------------------------------------------------------------------
// turbo::Uniform<T>(tag, bitgen, lo, hi)
// -----------------------------------------------------------------------------
//
// `turbo::Uniform()` produces random values of type `T` uniformly distributed in
// a defined interval {lo, hi}. The interval `tag` defines the type of interval
// which should be one of the following possible values:
//
//   * `turbo::IntervalOpenOpen`
//   * `turbo::IntervalOpenClosed`
//   * `turbo::IntervalClosedOpen`
//   * `turbo::IntervalClosedClosed`
//
// where "open" refers to an exclusive value (excluded) from the output, while
// "closed" refers to an inclusive value (included) from the output.
//
// In the absence of an explicit return type `T`, `turbo::Uniform()` will deduce
// the return type based on the provided endpoint arguments {A lo, B hi}.
// Given these endpoints, one of {A, B} will be chosen as the return type, if
// a type can be implicitly converted into the other in a lossless way. The
// lack of any such implicit conversion between {A, B} will produce a
// compile-time error
//
// See https://en.wikipedia.org/wiki/Uniform_distribution_(continuous)
//
// Example:
//
//   turbo::BitGen bitgen;
//
//   // Produce a random float value between 0.0 and 1.0, inclusive
//   auto x = turbo::Uniform(turbo::IntervalClosedClosed, bitgen, 0.0f, 1.0f);
//
//   // The most common interval of `turbo::IntervalClosedOpen` is available by
//   // default:
//
//   auto x = turbo::Uniform(bitgen, 0.0f, 1.0f);
//
//   // Return-types are typically inferred from the arguments, however callers
//   // can optionally provide an explicit return-type to the template.
//
//   auto x = turbo::Uniform<float>(bitgen, 0, 1);
//
template <typename R = void, typename TagType, typename URBG>
typename turbo::enable_if_t<!std::is_same<R, void>::value, R>  //
Uniform(TagType tag,
        URBG&& urbg,  // NOLINT(runtime/references)
        R lo, R hi) {
  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = random_internal::UniformDistributionWrapper<R>;

  auto a = random_internal::uniform_lower_bound(tag, lo, hi);
  auto b = random_internal::uniform_upper_bound(tag, lo, hi);
  if (!random_internal::is_uniform_range_valid(a, b)) return lo;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, tag, lo, hi);
}

// turbo::Uniform<T>(bitgen, lo, hi)
//
// Overload of `Uniform()` using the default closed-open interval of [lo, hi),
// and returning values of type `T`
template <typename R = void, typename URBG>
typename turbo::enable_if_t<!std::is_same<R, void>::value, R>  //
Uniform(URBG&& urbg,  // NOLINT(runtime/references)
        R lo, R hi) {
  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = random_internal::UniformDistributionWrapper<R>;
  constexpr auto tag = turbo::IntervalClosedOpen;

  auto a = random_internal::uniform_lower_bound(tag, lo, hi);
  auto b = random_internal::uniform_upper_bound(tag, lo, hi);
  if (!random_internal::is_uniform_range_valid(a, b)) return lo;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, lo, hi);
}

// turbo::Uniform(tag, bitgen, lo, hi)
//
// Overload of `Uniform()` using different (but compatible) lo, hi types. Note
// that a compile-error will result if the return type cannot be deduced
// correctly from the passed types.
template <typename R = void, typename TagType, typename URBG, typename A,
          typename B>
typename turbo::enable_if_t<std::is_same<R, void>::value,
                           random_internal::uniform_inferred_return_t<A, B>>
Uniform(TagType tag,
        URBG&& urbg,  // NOLINT(runtime/references)
        A lo, B hi) {
  using gen_t = turbo::decay_t<URBG>;
  using return_t = typename random_internal::uniform_inferred_return_t<A, B>;
  using distribution_t = random_internal::UniformDistributionWrapper<return_t>;

  auto a = random_internal::uniform_lower_bound<return_t>(tag, lo, hi);
  auto b = random_internal::uniform_upper_bound<return_t>(tag, lo, hi);
  if (!random_internal::is_uniform_range_valid(a, b)) return lo;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, tag, static_cast<return_t>(lo),
                      static_cast<return_t>(hi));
}

// turbo::Uniform(bitgen, lo, hi)
//
// Overload of `Uniform()` using different (but compatible) lo, hi types and the
// default closed-open interval of [lo, hi). Note that a compile-error will
// result if the return type cannot be deduced correctly from the passed types.
template <typename R = void, typename URBG, typename A, typename B>
typename turbo::enable_if_t<std::is_same<R, void>::value,
                           random_internal::uniform_inferred_return_t<A, B>>
Uniform(URBG&& urbg,  // NOLINT(runtime/references)
        A lo, B hi) {
  using gen_t = turbo::decay_t<URBG>;
  using return_t = typename random_internal::uniform_inferred_return_t<A, B>;
  using distribution_t = random_internal::UniformDistributionWrapper<return_t>;

  constexpr auto tag = turbo::IntervalClosedOpen;
  auto a = random_internal::uniform_lower_bound<return_t>(tag, lo, hi);
  auto b = random_internal::uniform_upper_bound<return_t>(tag, lo, hi);
  if (!random_internal::is_uniform_range_valid(a, b)) return lo;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, static_cast<return_t>(lo),
                      static_cast<return_t>(hi));
}

// turbo::Uniform<unsigned T>(bitgen)
//
// Overload of Uniform() using the minimum and maximum values of a given type
// `T` (which must be unsigned), returning a value of type `unsigned T`
template <typename R, typename URBG>
typename turbo::enable_if_t<!std::numeric_limits<R>::is_signed, R>  //
Uniform(URBG&& urbg) {  // NOLINT(runtime/references)
  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = random_internal::UniformDistributionWrapper<R>;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg);
}

// -----------------------------------------------------------------------------
// turbo::Bernoulli(bitgen, p)
// -----------------------------------------------------------------------------
//
// `turbo::Bernoulli` produces a random boolean value, with probability `p`
// (where 0.0 <= p <= 1.0) equaling `true`.
//
// Prefer `turbo::Bernoulli` to produce boolean values over other alternatives
// such as comparing an `turbo::Uniform()` value to a specific output.
//
// See https://en.wikipedia.org/wiki/Bernoulli_distribution
//
// Example:
//
//   turbo::BitGen bitgen;
//   ...
//   if (turbo::Bernoulli(bitgen, 1.0/3721.0)) {
//     std::cout << "Asteroid field navigation successful.";
//   }
//
template <typename URBG>
bool Bernoulli(URBG&& urbg,  // NOLINT(runtime/references)
               double p) {
  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = turbo::bernoulli_distribution;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, p);
}

// -----------------------------------------------------------------------------
// turbo::Beta<T>(bitgen, alpha, beta)
// -----------------------------------------------------------------------------
//
// `turbo::Beta` produces a floating point number distributed in the closed
// interval [0,1] and parameterized by two values `alpha` and `beta` as per a
// Beta distribution. `T` must be a floating point type, but may be inferred
// from the types of `alpha` and `beta`.
//
// See https://en.wikipedia.org/wiki/Beta_distribution.
//
// Example:
//
//   turbo::BitGen bitgen;
//   ...
//   double sample = turbo::Beta(bitgen, 3.0, 2.0);
//
template <typename RealType, typename URBG>
RealType Beta(URBG&& urbg,  // NOLINT(runtime/references)
              RealType alpha, RealType beta) {
  static_assert(
      std::is_floating_point<RealType>::value,
      "Template-argument 'RealType' must be a floating-point type, in "
      "turbo::Beta<RealType, URBG>(...)");

  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = typename turbo::beta_distribution<RealType>;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, alpha, beta);
}

// -----------------------------------------------------------------------------
// turbo::Exponential<T>(bitgen, lambda = 1)
// -----------------------------------------------------------------------------
//
// `turbo::Exponential` produces a floating point number representing the
// distance (time) between two consecutive events in a point process of events
// occurring continuously and independently at a constant average rate. `T` must
// be a floating point type, but may be inferred from the type of `lambda`.
//
// See https://en.wikipedia.org/wiki/Exponential_distribution.
//
// Example:
//
//   turbo::BitGen bitgen;
//   ...
//   double call_length = turbo::Exponential(bitgen, 7.0);
//
template <typename RealType, typename URBG>
RealType Exponential(URBG&& urbg,  // NOLINT(runtime/references)
                     RealType lambda = 1) {
  static_assert(
      std::is_floating_point<RealType>::value,
      "Template-argument 'RealType' must be a floating-point type, in "
      "turbo::Exponential<RealType, URBG>(...)");

  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = typename turbo::exponential_distribution<RealType>;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, lambda);
}

// -----------------------------------------------------------------------------
// turbo::Gaussian<T>(bitgen, mean = 0, stddev = 1)
// -----------------------------------------------------------------------------
//
// `turbo::Gaussian` produces a floating point number selected from the Gaussian
// (ie. "Normal") distribution. `T` must be a floating point type, but may be
// inferred from the types of `mean` and `stddev`.
//
// See https://en.wikipedia.org/wiki/Normal_distribution
//
// Example:
//
//   turbo::BitGen bitgen;
//   ...
//   double giraffe_height = turbo::Gaussian(bitgen, 16.3, 3.3);
//
template <typename RealType, typename URBG>
RealType Gaussian(URBG&& urbg,  // NOLINT(runtime/references)
                  RealType mean = 0, RealType stddev = 1) {
  static_assert(
      std::is_floating_point<RealType>::value,
      "Template-argument 'RealType' must be a floating-point type, in "
      "turbo::Gaussian<RealType, URBG>(...)");

  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = typename turbo::gaussian_distribution<RealType>;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, mean, stddev);
}

// -----------------------------------------------------------------------------
// turbo::LogUniform<T>(bitgen, lo, hi, base = 2)
// -----------------------------------------------------------------------------
//
// `turbo::LogUniform` produces random values distributed where the log to a
// given base of all values is uniform in a closed interval [lo, hi]. `T` must
// be an integral type, but may be inferred from the types of `lo` and `hi`.
//
// I.e., `LogUniform(0, n, b)` is uniformly distributed across buckets
// [0], [1, b-1], [b, b^2-1] .. [b^(k-1), (b^k)-1] .. [b^floor(log(n, b)), n]
// and is uniformly distributed within each bucket.
//
// The resulting probability density is inversely related to bucket size, though
// values in the final bucket may be more likely than previous values. (In the
// extreme case where n = b^i the final value will be tied with zero as the most
// probable result.
//
// If `lo` is nonzero then this distribution is shifted to the desired interval,
// so LogUniform(lo, hi, b) is equivalent to LogUniform(0, hi-lo, b)+lo.
//
// See https://en.wikipedia.org/wiki/Reciprocal_distribution
//
// Example:
//
//   turbo::BitGen bitgen;
//   ...
//   int v = turbo::LogUniform(bitgen, 0, 1000);
//
template <typename IntType, typename URBG>
IntType LogUniform(URBG&& urbg,  // NOLINT(runtime/references)
                   IntType lo, IntType hi, IntType base = 2) {
  static_assert(random_internal::IsIntegral<IntType>::value,
                "Template-argument 'IntType' must be an integral type, in "
                "turbo::LogUniform<IntType, URBG>(...)");

  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = typename turbo::log_uniform_int_distribution<IntType>;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, lo, hi, base);
}

// -----------------------------------------------------------------------------
// turbo::Poisson<T>(bitgen, mean = 1)
// -----------------------------------------------------------------------------
//
// `turbo::Poisson` produces discrete probabilities for a given number of events
// occurring within a fixed interval within the closed interval [0, max]. `T`
// must be an integral type.
//
// See https://en.wikipedia.org/wiki/Poisson_distribution
//
// Example:
//
//   turbo::BitGen bitgen;
//   ...
//   int requests_per_minute = turbo::Poisson<int>(bitgen, 3.2);
//
template <typename IntType, typename URBG>
IntType Poisson(URBG&& urbg,  // NOLINT(runtime/references)
                double mean = 1.0) {
  static_assert(random_internal::IsIntegral<IntType>::value,
                "Template-argument 'IntType' must be an integral type, in "
                "turbo::Poisson<IntType, URBG>(...)");

  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = typename turbo::poisson_distribution<IntType>;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, mean);
}

// -----------------------------------------------------------------------------
// turbo::Zipf<T>(bitgen, hi = max, q = 2, v = 1)
// -----------------------------------------------------------------------------
//
// `turbo::Zipf` produces discrete probabilities commonly used for modelling of
// rare events over the closed interval [0, hi]. The parameters `v` and `q`
// determine the skew of the distribution. `T`  must be an integral type, but
// may be inferred from the type of `hi`.
//
// See http://mathworld.wolfram.com/ZipfDistribution.html
//
// Example:
//
//   turbo::BitGen bitgen;
//   ...
//   int term_rank = turbo::Zipf<int>(bitgen);
//
template <typename IntType, typename URBG>
IntType Zipf(URBG&& urbg,  // NOLINT(runtime/references)
             IntType hi = (std::numeric_limits<IntType>::max)(), double q = 2.0,
             double v = 1.0) {
  static_assert(random_internal::IsIntegral<IntType>::value,
                "Template-argument 'IntType' must be an integral type, in "
                "turbo::Zipf<IntType, URBG>(...)");

  using gen_t = turbo::decay_t<URBG>;
  using distribution_t = typename turbo::zipf_distribution<IntType>;

  return random_internal::DistributionCaller<gen_t>::template Call<
      distribution_t>(&urbg, hi, q, v);
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_RANDOM_DISTRIBUTIONS_H_
