
#include "turbo/simd/simd.h"

namespace ts = turbo::simd;

template<class T, class Arch>
ts::batch<T, Arch> mean(ts::batch<T, Arch> lhs, ts::batch<T, Arch> rhs) {
    return (lhs + rhs) / 2;
}
