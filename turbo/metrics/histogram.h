
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_METRICS_HISTOGRAM_H_
#define TURBO_METRICS_HISTOGRAM_H_

#include <vector>
#include <memory>
#include <map>
#include <string>
#include <string_view>
#include "turbo/metrics/counter.h"
#include "turbo/metrics/bucket.h"
#include "turbo/metrics/variable_base.h"

namespace turbo {

    class histogram : public variable_base {
    public:

        histogram() : variable_base() {}

        histogram(const std::string &name,
                  const std::string_view &help,
                  const bucket &buckets,
                  const variable_base::tag_type &tags = variable_base::tag_type());

        ~histogram();

        int expose(const std::string &name,
                   const std::string_view &help,
                   const bucket &buckets,
                   const variable_base::tag_type &tags = variable_base::tag_type());

        int expose_as(const std::string &prefix, const std::string &name,
                      const std::string_view &help,
                      const bucket &buckets,
                      const variable_base::tag_type &tags = variable_base::tag_type());

        void observe(double value) noexcept;

        void describe(std::ostream &os, bool /*quote_string*/) const override {}

        void collect_metrics(cache_metrics &metric) const override;

        histogram &operator<<(double v) {
            observe(v);
            return *this;
        }

    private:
        void make_bucket(const std::string &name, const bucket &buckets);

    private:
        bucket _bucket_boundaries;
        std::vector<std::unique_ptr<counter<int64_t>>> _bucket_counts;
        counter<double> _sum;
    };

}  // namespace turbo

#endif  // TURBO_METRICS_HISTOGRAM_H_
