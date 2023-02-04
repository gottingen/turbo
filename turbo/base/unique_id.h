//
// Created by 李寅斌 on 2023/2/3.
//

#ifndef TURBO_BASE_UNIQUE_ID_H_
#define TURBO_BASE_UNIQUE_ID_H_

#include "turbo/platform/port.h"
#include <atomic>

namespace turbo {
TURBO_NAMESPACE_BEGIN

/**
@brief generates a program-wise unique id of the give type (thread-safe)
*/
template <typename T, std::enable_if_t<std::is_integral_v<T>, void>* = nullptr>
T unique_id() {
  static std::atomic<T> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed);
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_UNIQUE_ID_H_
