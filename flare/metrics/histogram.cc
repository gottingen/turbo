
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/metrics/histogram.h"

namespace flare {

    histogram::histogram(const std::string &name,
                         const std::string_view &help,
                         const bucket &buckets,
                         const variable_base::tag_type &tags)
            : _bucket_boundaries(buckets),
              _sum(name + "_sum", "", std::unordered_map<std::string, std::string>(), DISPLAY_NON) {
        FLARE_CHECK(std::is_sorted(std::begin(_bucket_boundaries),
                                   std::end(_bucket_boundaries)));
        make_bucket(name, buckets);
        variable_base::expose(name, help, tags, DISPLAY_ON_METRICS);
    }

    void histogram::make_bucket(const std::string &name, const bucket &buckets) {
        _bucket_boundaries = buckets;
        _bucket_counts.clear();
        std::unordered_map<std::string, std::string> empty;
        for (size_t i = 0; i < _bucket_boundaries.size(); ++i) {
            std::string n = name + "_" + std::to_string(i);
            _bucket_counts.push_back(std::unique_ptr<counter<int64_t>>(new counter<int64_t>(n, "", empty, DISPLAY_NON)));
        }
    }

    histogram::~histogram() {
        for (size_t i = 0; i < _bucket_boundaries.size(); ++i) {
            _bucket_counts[i]->hide();
        }
        _sum.hide();
        hide();
    }

    int histogram::expose(const std::string &name,
               const std::string_view &help,
               const bucket &buckets,
               const variable_base::tag_type &tags) {
        return expose_as("", name, help, buckets, tags);
    }

    int histogram::expose_as(const std::string &prefix, const std::string &name,
                  const std::string_view &help,
                  const bucket &buckets,
                  const variable_base::tag_type &tags) {
        _sum.expose(prefix + name + "_sum", "", std::unordered_map<std::string, std::string>(), DISPLAY_NON);
        make_bucket(prefix + name, buckets);
        FLARE_CHECK(std::is_sorted(std::begin(_bucket_boundaries),
                                   std::end(_bucket_boundaries)));
        return variable_base::expose_as(prefix, name, help, tags, DISPLAY_ON_METRICS);
    }


    void histogram::observe(const double value) noexcept {
        // TODO: determine bucket list size at which binary search would be faster
        const auto bucket_index = static_cast<std::size_t>(std::distance(
                _bucket_boundaries.begin(),
                std::find_if(
                        std::begin(_bucket_boundaries), std::end(_bucket_boundaries),
                        [value](const double boundary) { return boundary >= value; })));
        if (bucket_index < _bucket_boundaries.size()) {
            _sum << value;
            (*_bucket_counts[bucket_index]) << 1;
        }
    }

/*
    void histogram::describe_metrics(std::ostream &out, const flare::time_point *stamp) const {
        out << "# HELP " << _name << "\n";
        out << "# TYPE " << _name << " histogram\n";
        size_t cnt = 0;

        double last = -std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < _bucket_counts.size(); i++) {
            metrics_detail::write_head(out, _name, _tags, "_bucket", "le", _bucket_boundaries[i]);
            last = _bucket_boundaries[i];

            auto c = _bucket_counts[i]->get_value();
            out << c;
            cnt += c;
            metrics_detail::write_tail(out, stamp);
        }

        if (last != std::numeric_limits<double>::infinity()) {
            metrics_detail::write_head(out, _name, _tags, "_bucket", "le", "+Inf");
            out << cnt;
            metrics_detail::write_tail(out, stamp);
        }
        metrics_detail::write_head(out, _name, _tags, "_sum");
        metrics_detail::write_value(out, _sum.get_value());
        metrics_detail::write_tail(out, stamp);
        metrics_detail::write_head(out, _name, _tags, "_count");
        out << cnt;
        metrics_detail::write_tail(out, stamp);
    }
    */

    void histogram::collect_metrics(cache_metrics &metric) const {
        copy_metric_family(metric);
        metric.type = metrics_type::mt_histogram;

        auto cumulative_count = 0ULL;
        for (std::size_t i{0}; i < _bucket_counts.size(); ++i) {
            cumulative_count += _bucket_counts[i]->get_value();
            auto bucket = cache_metrics::cached_bucket{};
            bucket.cumulative_count = cumulative_count;
            bucket.upper_bound = (i == _bucket_boundaries.size()
                                  ? std::numeric_limits<double>::infinity()
                                  : _bucket_boundaries[i]);
            metric.histogram.bucket.push_back(std::move(bucket));
        }
        metric.histogram.sample_count = cumulative_count;
        metric.histogram.sample_sum = _sum.get_value();
    }
}  // namespace flare