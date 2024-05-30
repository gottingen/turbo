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

#include <turbo/container/internal/hashtablez_sampler.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/per_thread_tls.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/macros.h>
#include <turbo/base/no_destructor.h>
#include <turbo/base/optimization.h>
#include <turbo/debugging/stacktrace.h>
#include <turbo/memory/memory.h>
#include <turbo/profiling/internal/exponential_biased.h>
#include <turbo/profiling/internal/sample_recorder.h>
#include <turbo/synchronization/mutex.h>
#include <turbo/times/clock.h>
#include <turbo/utility/utility.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr int HashtablezInfo::kMaxStackDepth;
#endif

namespace {
TURBO_CONST_INIT std::atomic<bool> g_hashtablez_enabled{
    false
};
TURBO_CONST_INIT std::atomic<int32_t> g_hashtablez_sample_parameter{1 << 10};
std::atomic<HashtablezConfigListener> g_hashtablez_config_listener{nullptr};

#if defined(TURBO_INTERNAL_HASHTABLEZ_SAMPLE)
TURBO_PER_THREAD_TLS_KEYWORD turbo::profiling_internal::ExponentialBiased
    g_exponential_biased_generator;
#endif

void TriggerHashtablezConfigListener() {
  auto* listener = g_hashtablez_config_listener.load(std::memory_order_acquire);
  if (listener != nullptr) listener();
}

}  // namespace

#if defined(TURBO_INTERNAL_HASHTABLEZ_SAMPLE)
TURBO_PER_THREAD_TLS_KEYWORD SamplingState global_next_sample = {0, 0};
#endif  // defined(TURBO_INTERNAL_HASHTABLEZ_SAMPLE)

HashtablezSampler& GlobalHashtablezSampler() {
  static turbo::NoDestructor<HashtablezSampler> sampler;
  return *sampler;
}

HashtablezInfo::HashtablezInfo() = default;
HashtablezInfo::~HashtablezInfo() = default;

void HashtablezInfo::PrepareForSampling(int64_t stride,
                                        size_t inline_element_size_value,
                                        size_t key_size_value,
                                        size_t value_size_value,
                                        uint16_t soo_capacity_value) {
  capacity.store(0, std::memory_order_relaxed);
  size.store(0, std::memory_order_relaxed);
  num_erases.store(0, std::memory_order_relaxed);
  num_rehashes.store(0, std::memory_order_relaxed);
  max_probe_length.store(0, std::memory_order_relaxed);
  total_probe_length.store(0, std::memory_order_relaxed);
  hashes_bitwise_or.store(0, std::memory_order_relaxed);
  hashes_bitwise_and.store(~size_t{}, std::memory_order_relaxed);
  hashes_bitwise_xor.store(0, std::memory_order_relaxed);
  max_reserve.store(0, std::memory_order_relaxed);

  create_time = turbo::Time::current_time();
  weight = stride;
  // The inliner makes hardcoded skip_count difficult (especially when combined
  // with LTO).  We use the ability to exclude stacks by regex when encoding
  // instead.
  depth = turbo::GetStackTrace(stack, HashtablezInfo::kMaxStackDepth,
                              /* skip_count= */ 0);
  inline_element_size = inline_element_size_value;
  key_size = key_size_value;
  value_size = value_size_value;
  soo_capacity = soo_capacity_value;
}

static bool ShouldForceSampling() {
  enum ForceState {
    kDontForce,
    kForce,
    kUninitialized
  };
  TURBO_CONST_INIT static std::atomic<ForceState> global_state{
      kUninitialized};
  ForceState state = global_state.load(std::memory_order_relaxed);
  if (TURBO_PREDICT_TRUE(state == kDontForce)) return false;

  if (state == kUninitialized) {
    state = TURBO_INTERNAL_C_SYMBOL(TurboContainerInternalSampleEverything)()
                ? kForce
                : kDontForce;
    global_state.store(state, std::memory_order_relaxed);
  }
  return state == kForce;
}

HashtablezInfo* SampleSlow(SamplingState& next_sample,
                           size_t inline_element_size, size_t key_size,
                           size_t value_size, uint16_t soo_capacity) {
  if (TURBO_PREDICT_FALSE(ShouldForceSampling())) {
    next_sample.next_sample = 1;
    const int64_t old_stride = exchange(next_sample.sample_stride, 1);
    HashtablezInfo* result = GlobalHashtablezSampler().Register(
        old_stride, inline_element_size, key_size, value_size, soo_capacity);
    return result;
  }

#if !defined(TURBO_INTERNAL_HASHTABLEZ_SAMPLE)
  next_sample = {
      std::numeric_limits<int64_t>::max(),
      std::numeric_limits<int64_t>::max(),
  };
  return nullptr;
#else
  bool first = next_sample.next_sample < 0;

  const int64_t next_stride = g_exponential_biased_generator.GetStride(
      g_hashtablez_sample_parameter.load(std::memory_order_relaxed));

  next_sample.next_sample = next_stride;
  const int64_t old_stride = exchange(next_sample.sample_stride, next_stride);
  // Small values of interval are equivalent to just sampling next time.
  TURBO_ASSERT(next_stride >= 1);

  // g_hashtablez_enabled can be dynamically flipped, we need to set a threshold
  // low enough that we will start sampling in a reasonable time, so we just use
  // the default sampling rate.
  if (!g_hashtablez_enabled.load(std::memory_order_relaxed)) return nullptr;

  // We will only be negative on our first count, so we should just retry in
  // that case.
  if (first) {
    if (TURBO_PREDICT_TRUE(--next_sample.next_sample > 0)) return nullptr;
    return SampleSlow(next_sample, inline_element_size, key_size, value_size,
                      soo_capacity);
  }

  return GlobalHashtablezSampler().Register(old_stride, inline_element_size,
                                            key_size, value_size, soo_capacity);
#endif
}

void UnsampleSlow(HashtablezInfo* info) {
  GlobalHashtablezSampler().Unregister(info);
}

void RecordRehashSlow(HashtablezInfo* info, size_t total_probe_length) {
#ifdef TURBO_INTERNAL_HAVE_SSE2
  total_probe_length /= 16;
#else
  total_probe_length /= 8;
#endif
  info->total_probe_length.store(total_probe_length, std::memory_order_relaxed);
  info->num_erases.store(0, std::memory_order_relaxed);
  // There is only one concurrent writer, so `load` then `store` is sufficient
  // instead of using `fetch_add`.
  info->num_rehashes.store(
      1 + info->num_rehashes.load(std::memory_order_relaxed),
      std::memory_order_relaxed);
}

void RecordReservationSlow(HashtablezInfo* info, size_t target_capacity) {
  info->max_reserve.store(
      (std::max)(info->max_reserve.load(std::memory_order_relaxed),
                 target_capacity),
      std::memory_order_relaxed);
}

void RecordClearedReservationSlow(HashtablezInfo* info) {
  info->max_reserve.store(0, std::memory_order_relaxed);
}

void RecordStorageChangedSlow(HashtablezInfo* info, size_t size,
                              size_t capacity) {
  info->size.store(size, std::memory_order_relaxed);
  info->capacity.store(capacity, std::memory_order_relaxed);
  if (size == 0) {
    // This is a clear, reset the total/num_erases too.
    info->total_probe_length.store(0, std::memory_order_relaxed);
    info->num_erases.store(0, std::memory_order_relaxed);
  }
}

void RecordInsertSlow(HashtablezInfo* info, size_t hash,
                      size_t distance_from_desired) {
  // SwissTables probe in groups of 16, so scale this to count items probes and
  // not offset from desired.
  size_t probe_length = distance_from_desired;
#ifdef TURBO_INTERNAL_HAVE_SSE2
  probe_length /= 16;
#else
  probe_length /= 8;
#endif

  info->hashes_bitwise_and.fetch_and(hash, std::memory_order_relaxed);
  info->hashes_bitwise_or.fetch_or(hash, std::memory_order_relaxed);
  info->hashes_bitwise_xor.fetch_xor(hash, std::memory_order_relaxed);
  info->max_probe_length.store(
      std::max(info->max_probe_length.load(std::memory_order_relaxed),
               probe_length),
      std::memory_order_relaxed);
  info->total_probe_length.fetch_add(probe_length, std::memory_order_relaxed);
  info->size.fetch_add(1, std::memory_order_relaxed);
}

void RecordEraseSlow(HashtablezInfo* info) {
  info->size.fetch_sub(1, std::memory_order_relaxed);
  // There is only one concurrent writer, so `load` then `store` is sufficient
  // instead of using `fetch_add`.
  info->num_erases.store(1 + info->num_erases.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
}

void SetHashtablezConfigListener(HashtablezConfigListener l) {
  g_hashtablez_config_listener.store(l, std::memory_order_release);
}

bool IsHashtablezEnabled() {
  return g_hashtablez_enabled.load(std::memory_order_acquire);
}

void SetHashtablezEnabled(bool enabled) {
  SetHashtablezEnabledInternal(enabled);
  TriggerHashtablezConfigListener();
}

void SetHashtablezEnabledInternal(bool enabled) {
  g_hashtablez_enabled.store(enabled, std::memory_order_release);
}

int32_t GetHashtablezSampleParameter() {
  return g_hashtablez_sample_parameter.load(std::memory_order_acquire);
}

void SetHashtablezSampleParameter(int32_t rate) {
  SetHashtablezSampleParameterInternal(rate);
  TriggerHashtablezConfigListener();
}

void SetHashtablezSampleParameterInternal(int32_t rate) {
  if (rate > 0) {
    g_hashtablez_sample_parameter.store(rate, std::memory_order_release);
  } else {
    TURBO_RAW_LOG(ERROR, "Invalid hashtablez sample rate: %lld",
                 static_cast<long long>(rate));  // NOLINT(runtime/int)
  }
}

size_t GetHashtablezMaxSamples() {
  return GlobalHashtablezSampler().GetMaxSamples();
}

void SetHashtablezMaxSamples(size_t max) {
  SetHashtablezMaxSamplesInternal(max);
  TriggerHashtablezConfigListener();
}

void SetHashtablezMaxSamplesInternal(size_t max) {
  if (max > 0) {
    GlobalHashtablezSampler().SetMaxSamples(max);
  } else {
    TURBO_RAW_LOG(ERROR, "Invalid hashtablez max samples: 0");
  }
}

}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
