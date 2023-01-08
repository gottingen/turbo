//
//
// This test helper library contains a table of powers of 10, to guarantee
// precise values are computed across the full range of doubles. We can't rely
// on the pow() function, because not all standard libraries ship a version
// that is precise.
#ifndef TEST_TESTING_POW10_HELPER_H_
#define TEST_TESTING_POW10_HELPER_H_

#include <vector>

#include "flare/base/profile.h"

namespace flare {

    namespace strings_internal {

        // Computes the precise value of 10^exp. (I.e. the nearest representable
        // double to the exact value, rounding to nearest-even in the (single) case of
        // being exactly halfway between.)
        double Pow10(int exp);

    }  // namespace strings_internal

}  // namespace flare

#endif  // TEST_TESTING_POW10_HELPER_H_
