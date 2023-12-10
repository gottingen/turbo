
#include "turbo/simd/simd.h"

namespace xs = turbo::simd;

xs::batch<float> mean(xs::batch<float> lhs, xs::batch<float> rhs) {
    return (lhs + rhs) / 2;
}
