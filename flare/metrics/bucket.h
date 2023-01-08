
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_METRICS_BUCKET_H_
#define FLARE_METRICS_BUCKET_H_


#include <vector>
#include "flare/times/time.h"

namespace flare {
    typedef std::vector<double> bucket;

    struct bucket_builder {

        static bucket liner_values(double start, double width, size_t number);

        static bucket exponential_values(double start, double factor, size_t num);

        static bucket liner_duration(flare::duration start, flare::duration width, size_t num);

        static bucket exponential_duration(flare::duration start, uint64_t factor, size_t num);
    };
}
#endif  // FLARE_METRICS_BUCKET_H_
