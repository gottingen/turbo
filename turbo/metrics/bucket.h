
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_METRICS_BUCKET_H_
#define TURBO_METRICS_BUCKET_H_


#include <vector>
#include "turbo/times/time.h"

namespace turbo {
    typedef std::vector<double> bucket;

    struct bucket_builder {

        static bucket liner_values(double start, double width, size_t number);

        static bucket exponential_values(double start, double factor, size_t num);

        static bucket liner_duration(turbo::duration start, turbo::duration width, size_t num);

        static bucket exponential_duration(turbo::duration start, uint64_t factor, size_t num);
    };
}
#endif  // TURBO_METRICS_BUCKET_H_
