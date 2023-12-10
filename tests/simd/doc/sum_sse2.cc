
// compile with -msse2
#include "sum.h"

template float sum::operator()<turbo::simd::sse2, float>(turbo::simd::sse2, float const *, unsigned);
