//
// Created by 李寅斌 on 2023/2/5.
//

#ifndef TURBO_BASE_ASSUME_H_
#define TURBO_BASE_ASSUME_H_

#include <cstdlib>
#include "turbo/platform/port.h"

namespace turbo {

TURBO_FORCE_INLINE void assume(bool cond) {
#if defined(__clang__) // Must go first because Clang also defines __GNUC__.
  __builtin_assume(cond);
#elif defined(__GNUC__)
  if (!cond) {
    __builtin_unreachable();
  }
#elif defined(_MSC_VER)
  __assume(cond);
#else
  // Do nothing.
#endif
}

TURBO_FORCE_INLINE void assume_unreachable() {
  assume(false);
  // Do a bit more to get the compiler to understand
  // that this function really will never return.
#if defined(__GNUC__)
  __builtin_unreachable();
#elif defined(_MSC_VER)
  __assume(0);
#else
  // Well, it's better than nothing.
  std::abort();
#endif
}

} // namespace turbo

#endif // TURBO_BASE_ASSUME_H_
