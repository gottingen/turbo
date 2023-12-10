
// compile with -mavx2
#include "sum.h"

template float sum::operator()<turbo::simd::avx2, float>(turbo::simd::avx2, float const *, unsigned);
