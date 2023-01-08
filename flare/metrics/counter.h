
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_METRICS_COUNTER_H_
#define FLARE_METRICS_COUNTER_H_

#include "flare/metrics/variable_reducer.h"

namespace flare {

    template<typename T>
    class counter : public variable_reducer<T, metrics_detail::add_to<T>, metrics_detail::minus_from<T> > {
    public:
        typedef variable_reducer<T, metrics_detail::add_to<T>, metrics_detail::minus_from<T> > Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        counter() : Base() {}

        explicit counter(const std::string_view &name,
                         const std::string_view &help = "",
                         const variable_base::tag_type &tags = variable_base::tag_type(),
                         display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)) : Base() {
            this->expose(name, help, tags, f);
        }

        counter(const std::string_view &prefix,
                const std::string_view &name,
                const std::string_view &help = "",
                const variable_base::tag_type &tags = variable_base::tag_type(),
                display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)) : Base() {
            this->expose_as(prefix, name, help, tags, f);
        }

        void collect_metrics(cache_metrics &metric) const override {
            this->copy_metric_family(metric);
            metric.type = metrics_type::mt_counter;
            metric.counter.value = this->get_value();
        }

        ~counter() { variable_base::hide(); }
    };


}  // namespace flare

#endif  // FLARE_METRICS_COUNTER_H_
