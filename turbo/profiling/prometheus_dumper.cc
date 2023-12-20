// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// Created by jeff on 23-12-21.
//

#include "turbo/profiling/prometheus_dumper.h"
#include <ostream>
#include <sstream>

namespace turbo {

    namespace profiling_internal {
        void write_value(std::ostream &out, double value) {
            if (std::isnan(value)) {
                out << "Nan";
            } else if (std::isinf(value)) {
                out << (value < 0 ? "-Inf" : "+Inf");
            } else {
                auto saved_flags = out.setf(std::ios::fixed, std::ios::floatfield);
                out << value;
                out.setf(saved_flags, std::ios::floatfield);
            }
        }

        void write_value(std::ostream &out, const std::string &value) {
            for (auto c: value) {
                if (c == '\\' || c == '"' || c == '\n') {
                    out << "\\";
                }
                out << c;
            }
        }


        // Write a line header: metric name and labels
        template<typename T = std::string>
        void write_head(std::ostream &out, const SnapshotFamily *metric,
                        const std::string &suffix = "",
                        const std::string &extraLabelName = "",
                        const T &extraLabelValue = T()) {
            out << metric->name << suffix;
            if (!metric->labels.empty() || !extraLabelName.empty()) {
                out << "{";
                const char *prefix = "";

                for (auto it = metric->labels.begin(); it != metric->labels.end(); it++) {
                    out << prefix << it->first << "=\"";
                    write_value(out, it->second);
                    out << "\"";
                    prefix = ",";
                }
                if (!extraLabelName.empty()) {
                    out << prefix << extraLabelName << "=\"";
                    write_value(out, extraLabelValue);
                    out << "\"";
                }
                out << "}";
            }
            out << " ";
        }

        // Write a line trailer: timestamp
        void write_tail(std::ostream &out, const SnapshotFamily *metrics) {
            if(metrics) {
                out << " " << metrics->timestamp_ms;
            }
            out << "\n";
        }

        void format_counter(std::ostream &out, const CounterSnapshot *metric) {
            out<< "# HELP "<< metric->name<<" "<<metric->description << "\n";
            out << "# TYPE " << metric->name << " counter\n";
            write_head(out, metric);
            write_value(out, metric->value);
            write_tail(out, metric);
        }

        void format_gauge(std::ostream &out,
                             const GaugeSnapshot *metric) {
            out<< "# HELP "<< metric->name<<" "<<metric->description << "\n";
            out<< "# TYPE " << metric->name << " gauge\n";
            write_head(out, metric);
            write_value(out, metric->value);
            write_tail(out, metric);
        }

        void format_histogram(std::ostream &out,
                                 const HistogramSnapshot *metric) {
            out<< "# HELP "<< metric->name<<" "<<metric->description << "\n";
            out<< "# TYPE " << metric->name << " histogram\n";
            write_head(out, metric, "_count");
            out << metric->count;
            write_tail(out, metric);

            write_head(out, metric, "_sum");
            write_value(out, metric->sum);
            write_tail(out, metric);

            double last = -std::numeric_limits<double>::infinity();
            size_t i = 0;
            for (auto &b : metric->boundaries) {
                write_head(out, metric, "_bucket", "le", b);
                last = b;
                out << metric->bins[i++];
                write_tail(out, metric);
            }

            if (last != std::numeric_limits<double>::infinity()) {
                write_head(out, metric, "_bucket", "le", "+Inf");
                out << metric->count;
                write_tail(out, metric);
            }
        }


    }  // namespace profiling_internal

    std::string PrometheusDumper::dump(const VariableSnapshot &snapshot) {
        std::stringstream ss;
        const CounterSnapshot *int_counter = std::get_if<CounterSnapshot>(&snapshot);
        if (int_counter) {
            profiling_internal::format_counter(ss, int_counter);
            return ss.str();
        }

        const GaugeSnapshot *int_gauge = std::get_if<GaugeSnapshot>(&snapshot);
        if (int_gauge) {
            profiling_internal::format_gauge(ss, int_gauge);
            return ss.str();
        }

        const HistogramSnapshot *int_histogram = std::get_if<HistogramSnapshot>(&snapshot);
        if (int_histogram) {
            profiling_internal::format_histogram(ss, int_histogram);
            return ss.str();
        }
        return "not support";
    }

}  // namespace turbo