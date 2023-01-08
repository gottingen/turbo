
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/metrics/prometheus_dumper.h"

namespace flare {

    namespace metrics_detail {
        // Write a double as a string, with proper formatting for infinity and NaN
        void write_value(cord_buf_builder *out, double value) {
            if (std::isnan(value)) {
                *out << "Nan";
            } else if (std::isinf(value)) {
                *out << (value < 0 ? "-Inf" : "+Inf");
            } else {
                auto saved_flags = out->setf(std::ios::fixed, std::ios::floatfield);
                *out << value;
                out->setf(saved_flags, std::ios::floatfield);
            }
        }

        void write_value(cord_buf_builder *out, const std::string &value) {
            for (auto c : value) {
                if (c == '\\' || c == '"' || c == '\n') {
                    *out << "\\";
                }
                *out << c;
            }
        }


        // Write a line header: metric name and labels
        template<typename T = std::string>
        void write_head(cord_buf_builder *out, const cache_metrics &metric,
                        const std::string &suffix = "",
                        const std::string &extraLabelName = "",
                        const T &extraLabelValue = T()) {
            *out << metric.name << suffix;
            if (!metric.tags.empty() || !extraLabelName.empty()) {
                *out << "{";
                const char *prefix = "";

                for (auto it = metric.tags.begin(); it != metric.tags.end(); it++) {
                    *out << prefix << it->first << "=\"";
                    write_value(out, it->second);
                    *out << "\"";
                    prefix = ",";
                }
                if (!extraLabelName.empty()) {
                    *out << prefix << extraLabelName << "=\"";
                    write_value(out, extraLabelValue);
                    *out << "\"";
                }
                *out << "}";
            }
            *out << " ";
        }

        // Write a line trailer: timestamp
        void write_tail(cord_buf_builder *out, const cache_metrics &metrics, const flare::time_point *tp) {
            if (tp) {
                *out << " " << tp->to_unix_millis();
            }
            *out << "\n";
        }

        void serialize_counter(cord_buf_builder *out, const cache_metrics &metric, const flare::time_point *tp) {
            write_head(out, metric);
            write_value(out, metric.counter.value);
            write_tail(out, metric, tp);
        }

        void serialize_gauge(cord_buf_builder *out,
                             const cache_metrics &metric, const flare::time_point *tp) {
            write_head(out, metric);
            write_value(out, metric.gauge.value);
            write_tail(out, metric, tp);
        }

        void serialize_histogram(cord_buf_builder *out,
                                 const cache_metrics &metric, const flare::time_point *tp) {
            auto &hist = metric.histogram;
            write_head(out, metric, "_count");
            *out << hist.sample_count;
            write_tail(out, metric, nullptr);

            write_head(out, metric, "_sum");
            write_value(out, hist.sample_sum);
            write_tail(out, metric, nullptr);

            double last = -std::numeric_limits<double>::infinity();
            for (auto &b : hist.bucket) {
                write_head(out, metric, "_bucket", "le", b.upper_bound);
                last = b.upper_bound;
                *out << b.cumulative_count;
                write_tail(out, metric, nullptr);
            }

            if (last != std::numeric_limits<double>::infinity()) {
                write_head(out, metric, "_bucket", "le", "+Inf");
                *out << hist.sample_count;
                write_tail(out, metric, nullptr);
            }
        }

    }

    bool prometheus_dumper::dump(const cache_metrics &metric, const flare::time_point *tp) {
        *_buf << "# HELP " << metric.help << "\n";

        switch (metric.type) {
            case metrics_type::mt_counter:
                *_buf << "# TYPE " << metric.name << " counter\n";
                metrics_detail::serialize_counter(_buf, metric, tp);
                break;
            case metrics_type::mt_gauge:
                *_buf << "# TYPE " << metric.name << " gauge\n";
                metrics_detail::serialize_gauge(_buf, metric, tp);
                break;
            case metrics_type::mt_histogram:
                *_buf << "# TYPE " << metric.name << " histogram\n";
                metrics_detail::serialize_histogram(_buf, metric, tp);
                break;
            case metrics_type::mt_timer:
                *_buf << "# TYPE " << metric.name << " histogram\n";
                metrics_detail::serialize_histogram(_buf, metric, tp);
                break;
            default:
                break;
        }
        return true;
    }

    std::string prometheus_dumper::dump_to_string(const cache_metrics &metric, const flare::time_point *tp) {
        flare::cord_buf_builder out;
        prometheus_dumper dumper(&out);
        dumper.dump(metric, tp);
        return out.buf().to_string();
    }
}  // namespace flare