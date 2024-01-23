// Copyright 2023 The Turbo Authors.
//
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

#ifndef  TURBO_VAR_LATENCY_RECORDER_H_
#define  TURBO_VAR_LATENCY_RECORDER_H_

#include "turbo/var/recorder.h"
#include "turbo/var/reducer.h"
#include "turbo/var/passive_status.h"
#include "turbo/var/internal/percentile.h"

namespace turbo::var_internal {

    class Percentile;

    typedef Window<IntRecorder, SERIES_IN_SECOND> RecorderWindow;
    typedef Window<Maxer<int64_t>, SERIES_IN_SECOND> MaxWindow;
    typedef Window<Percentile, SERIES_IN_SECOND> PercentileWindow;


    class CDF : public Variable {
    public:
        explicit CDF(PercentileWindow *w);

        ~CDF();

        void describe(std::ostream &os, bool quote_string) const override;

        int describe_series(std::ostream &os, const SeriesOptions &options) const override;

    private:
        PercentileWindow *_w;
    };

    // For mimic constructor inheritance.
    class LatencyRecorderBase {
    public:
        explicit LatencyRecorderBase(time_t window_size);

        time_t window_size() const { return _latency_window.window_size(); }

    protected:
        IntRecorder _latency;
        Maxer<int64_t> _max_latency;
        Percentile _latency_percentile;

        RecorderWindow _latency_window;
        MaxWindow _max_latency_window;
        PassiveStatus<int64_t> _count;
        PassiveStatus<int64_t> _qps;
        PercentileWindow _latency_percentile_window;
        PassiveStatus<int64_t> _latency_p1;
        PassiveStatus<int64_t> _latency_p2;
        PassiveStatus<int64_t> _latency_p3;
        PassiveStatus<int64_t> _latency_999;  // 99.9%
        PassiveStatus<int64_t> _latency_9999; // 99.99%
        CDF _latency_cdf;
        PassiveStatus<Batch<int64_t, 4> > _latency_percentiles;
    };
} // namespace turbo::var_internal
namespace turbo {

    class LatencyRecorder : public var_internal::LatencyRecorderBase {
        typedef var_internal::LatencyRecorderBase Base;
    public:
        LatencyRecorder() : Base(-1) {}

        explicit LatencyRecorder(time_t window_size) : Base(window_size) {}

        explicit LatencyRecorder(const std::string &prefix) : Base(-1) {
            expose(prefix);
        }

        LatencyRecorder(const std::string &prefix,
                        time_t window_size) : Base(window_size) {
            expose(prefix);
        }

        LatencyRecorder(const std::string &prefix1,
                        const std::string &prefix2) : Base(-1) {
            expose(prefix1, prefix2);
        }

        LatencyRecorder(const std::string &prefix1,
                        const std::string &prefix2,
                        time_t window_size) : Base(window_size) {
            expose(prefix1, prefix2);
        }

        ~LatencyRecorder() { hide(); }

        // Record the latency.
        LatencyRecorder &operator<<(int64_t latency);

        // Expose all internal variables using `prefix' as prefix.
        // Returns 0 on success, -1 otherwise.
        // Example:
        //   LatencyRecorder rec;
        //   rec.expose("foo_bar_write");     // foo_bar_write_latency
        //                                    // foo_bar_write_max_latency
        //                                    // foo_bar_write_count
        //                                    // foo_bar_write_qps
        //   rec.expose("foo_bar", "read");   // foo_bar_read_latency
        //                                    // foo_bar_read_max_latency
        //                                    // foo_bar_read_count
        //                                    // foo_bar_read_qps
        int expose(const std::string_view &prefix) {
            return expose(std::string_view(), prefix);
        }

        int expose(const std::string_view &prefix1,
                   const std::string_view &prefix2);

        // Hide all internal variables, called in dtor as well.
        void hide();

        // Get the average latency in recent |window_size| seconds
        // If |window_size| is absent, use the window_size to ctor.
        int64_t latency(time_t window_size) const { return _latency_window.get_value(window_size).get_average_int(); }

        int64_t latency() const { return _latency_window.get_value().get_average_int(); }

        // Get p1/p2/p3/99.9-ile latencies in recent window_size-to-ctor seconds.
        Batch<int64_t, 4> latency_percentiles() const;

        // Get the max latency in recent window_size-to-ctor seconds.
        int64_t max_latency() const { return _max_latency_window.get_value(); }

        // Get the total number of recorded latencies.
        int64_t count() const { return _latency.get_value().num; }

        // Get qps in recent |window_size| seconds. The `q' means latencies
        // recorded by operator<<().
        // If |window_size| is absent, use the window_size to ctor.
        int64_t qps(time_t window_size) const;

        int64_t qps() const { return _qps.get_value(); }

        // Get |ratio|-ile latency in recent |window_size| seconds
        // E.g. 0.99 means 99%-ile
        int64_t latency_percentile(double ratio) const;

        // Get name of a sub-var.
        const std::string &latency_name() const { return _latency_window.name(); }

        const std::string &latency_percentiles_name() const { return _latency_percentiles.name(); }

        const std::string &latency_cdf_name() const { return _latency_cdf.name(); }

        const std::string &max_latency_name() const { return _max_latency_window.name(); }

        const std::string &count_name() const { return _count.name(); }

        const std::string &qps_name() const { return _qps.name(); }
    };

    std::ostream &operator<<(std::ostream &os, const LatencyRecorder &);

}  // namespace turbo

#endif  //TURBO_VAR_LATENCY_RECORDER_H_
