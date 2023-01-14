
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/metrics/bucket.h"
#include "turbo/base/profile.h"
#include "turbo/log/logging.h"

namespace turbo {

    bucket bucket_builder::liner_values(double start, double width, size_t number) {
        TURBO_CHECK(width > 0);
        std::vector<double> ret;
        ret.reserve(number);
        double value = start;
        for (size_t i = 0; i < number; ++i) {
            ret.push_back(value);
            value += width;
        }
        return ret;
    }

    bucket bucket_builder::exponential_values(double start, double factor, size_t number) {
        TURBO_CHECK(factor > 0);
        TURBO_CHECK(start > 0);
        std::vector<double> ret;
        ret.reserve(number);
        double value = start;
        for (size_t i = 0; i < number; ++i) {
            ret.push_back(value);
            value *= factor;
        }
        return ret;
    }

    bucket bucket_builder::liner_duration(turbo::duration start, turbo::duration width, size_t num) {
        turbo::duration value = start;
        TURBO_CHECK(width.to_double_microseconds() > 0);
        std::vector<double> ret;
        ret.reserve(num);
        for (size_t i = 0; i < num; ++i) {
            ret.push_back(value.to_double_microseconds());
            value += width;
        }
        return ret;
    }

    bucket bucket_builder::exponential_duration(turbo::duration start, uint64_t factor, size_t num) {
        turbo::duration value = start;
        TURBO_CHECK(factor > 1);
        std::vector<double> ret;
        ret.reserve(num);
        for (size_t i = 0; i < num; ++i) {
            ret.push_back(value.to_double_microseconds());
            value *= factor;
        }
        return ret;
    }

}  // namespace abel
