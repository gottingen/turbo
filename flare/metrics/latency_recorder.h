
#ifndef  FLARE_VARIABLE_LATENCY_RECORDER_H_
#define  FLARE_VARIABLE_LATENCY_RECORDER_H_

#include "flare/metrics/recorder.h"
#include "flare/metrics/variable_reducer.h"
#include "flare/metrics/gauge.h"
#include "flare/metrics/gauge.h"
#include "flare/metrics/detail/percentile.h"

namespace flare {
    namespace metrics_detail {

        class percentile;

        typedef window<IntRecorder, SERIES_IN_SECOND> RecorderWindow;
        typedef window<max_gauge<int64_t>, SERIES_IN_SECOND> MaxWindow;
        typedef window<percentile, SERIES_IN_SECOND> PercentileWindow;

        // NOTE: Always use int64_t in the interfaces no matter what the impl. is.

        class CDF : public variable_base {
        public:
            explicit CDF(PercentileWindow *w);

            ~CDF();

            void describe(std::ostream &os, bool quote_string) const override;

            int describe_series(std::ostream &os, const variable_series_options &options) const override;

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
            max_gauge<int64_t> _max_latency;
            percentile _latency_percentile;

            RecorderWindow _latency_window;
            MaxWindow _max_latency_window;
            status_gauge<int64_t> _count;
            status_gauge<int64_t> _qps;
            PercentileWindow _latency_percentile_window;
            status_gauge<int64_t> _latency_p1;
            status_gauge<int64_t> _latency_p2;
            status_gauge<int64_t> _latency_p3;
            status_gauge<int64_t> _latency_999;  // 99.9%
            status_gauge<int64_t> _latency_9999; // 99.99%
            CDF _latency_cdf;
            status_gauge<Vector<int64_t, 4> > _latency_percentiles;
        };
    } // namespace detail

    // Specialized structure to record latency.
    // It's not a variable_base, but it contains multiple variable inside.
    class LatencyRecorder : public metrics_detail::LatencyRecorderBase {
        typedef metrics_detail::LatencyRecorderBase Base;
    public:
        LatencyRecorder() : Base(-1) {}

        explicit LatencyRecorder(time_t window_size) : Base(window_size) {}

        explicit LatencyRecorder(const std::string_view &prefix) : Base(-1) {
            expose(prefix);
        }

        LatencyRecorder(const std::string_view &prefix,
                        time_t window_size) : Base(window_size) {
            expose(prefix);
        }

        LatencyRecorder(const std::string_view &prefix1,
                        const std::string_view &prefix2) : Base(-1) {
            expose(prefix1, prefix2);
        }

        LatencyRecorder(const std::string_view &prefix1,
                        const std::string_view &prefix2,
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
        Vector<int64_t, 4> latency_percentiles() const;

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

        // Get name of a sub-variable.
        const std::string &latency_name() const { return _latency_window.name(); }

        const std::string &latency_percentiles_name() const { return _latency_percentiles.name(); }

        const std::string &latency_cdf_name() const { return _latency_cdf.name(); }

        const std::string &max_latency_name() const { return _max_latency_window.name(); }

        const std::string &count_name() const { return _count.name(); }

        const std::string &qps_name() const { return _qps.name(); }
    };

    std::ostream &operator<<(std::ostream &os, const LatencyRecorder &);

}  // namespace flare

#endif  // FLARE_VARIABLE_LATENCY_RECORDER_H_
