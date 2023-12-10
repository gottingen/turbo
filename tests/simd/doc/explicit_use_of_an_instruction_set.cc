
#include "turbo/simd/simd.h"
#include <iostream>

namespace ts = turbo::simd;

int main(int argc, char *argv[]) {
    ts::batch<double, ts::avx> a = {1.5, 2.5, 3.5, 4.5};
    ts::batch<double, ts::avx> b = {2.5, 3.5, 4.5, 5.5};
    auto mean = (a + b) / 2;
    std::cout << mean << std::endl;
    return 0;
}
