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


#ifndef TURBO_PROFILING_HISTOGRAM_H_
#define TURBO_PROFILING_HISTOGRAM_H_

#include "turbo/profiling/variable.h"
#include "turbo/profiling/internal/operators.h"
#include "turbo/profiling/internal/batch_combiner.h"
#include "turbo/profiling/internal/batch_reducer.h"
#include "turbo/profiling/variable.h"
#include "turbo/times/clock.h"
#include "turbo/meta/reflect.h"
#include "turbo/log/logging.h"
#include <array>
#include <atomic>
#include <vector>

namespace turbo {

    template<typename T>
    struct HistogramResult {
        std::vector<T> boundaries;
        std::vector<T> bins;
        T sum;
        T avg;
        size_t count;
    };

    struct ToMilliseconds {
        int64_t operator()(const turbo::Duration &d) const {
            return turbo::to_int64_milliseconds(d);
        }
    };

    struct ToMicroseconds {
        int64_t operator()(const turbo::Duration &d) const {
            return turbo::to_int64_microseconds(d);
        }
    };

    struct ToNanoseconds {
        int64_t operator()(const turbo::Duration &d) const {
            return turbo::to_int64_nanoseconds(d);
        }
    };

    struct ToSeconds {
        int64_t operator()(const turbo::Duration &d) const {
            return turbo::to_int64_seconds(d);
        }
    };

    struct ToDoubleSeconds {
        double operator()(const turbo::Duration &d) const {
            return turbo::to_double_seconds(d);
        }
    };

    struct ToDoubleMilliseconds {
        double operator()(const turbo::Duration &d) const {
            return turbo::to_double_milliseconds(d);
        }
    };

    struct ToDoubleMicroseconds {
        double operator()(const turbo::Duration &d) const {
            return turbo::to_double_microseconds(d);
        }
    };

    struct ToDoubleNanoseconds {
        double operator()(const turbo::Duration &d) const {
            return turbo::to_double_nanoseconds(d);
        }
    };

    struct ToDoubleMinutes {
        double operator()(const turbo::Duration &d) const {
            return turbo::to_double_minutes(d);
        }
    };




    template<typename H, typename Op>
    class ScopeLatency {
    public:
        explicit ScopeLatency(H *histogram, Op op) : _op(op), _start(turbo::time_now()), _histogram(histogram) {}

        ~ScopeLatency() {
            auto end = turbo::time_now();
            auto duration = end - _start;
            _histogram->add_value(_op(duration));
        }

    private:
        Op _op;
        turbo::Time _start;
        H *_histogram;
    };

    /**
     * @ingroup turbo_profiling_histograms
     * @brief A histogram that keeps the distribution of values in a set of bins.
     *        The histogram is aggregated using the reducer.
     *        histogram  also supports the scope latency, the latency is the time between the scope creation and
     *        the scope destruction. the latency is added to the histogram.
     *        Example:
     *        @code
     *        Histogram<int, 10> histogram;
     *        histogram.set_boundaries({10, 20, 30, 40, 50, 60, 70, 80, 90, 100});
     *        // mesure the latency in milliseconds
     *        void deal_rpc(Request *req) {
     *              scope_reacord = histogram.scope_latency_milliseconds();
     *              // do something
     *              // the latency is added to the histogram
     *              // the latency is the time between the scope creation and the scope destruction.
     *              // the latency is added to the histogram.
     *              // the latency is the time between the scope creation and the scope destruction.
     *         }
     *         @endcode
     * @tparam T The type of the value to be stored.
     * @tparam N The number of bins.
     */
    template<typename T, size_t N, typename = typename std::enable_if_t<is_atomical<T>::value>>
    class Histogram : public Variable {
    public:
        typedef profiling_internal::BatchReducer<T, N + 1, profiling_internal::AddTo<T>,
                profiling_internal::AddTo<T>> reducer_type;
        static constexpr VariableAttr kHistogramAttr = VariableAttr(DUMP_PROMETHEUS_TYPE, VariableType::VT_HISTOGRAM);
    public:
        Histogram()
                : Variable(), _bins(), _reducer(), _status(unavailable_error("")) {}

        explicit Histogram(const std::string_view &name, const std::string_view &description = "")
                : Histogram() {
            _status = this->expose(name, description, {}, kHistogramAttr);
        }

        Histogram(const std::string_view &name, const std::string_view &description,
                  const std::map<std::string, std::string> &tags)
                : Histogram() {
            _status = this->expose(name, description, tags, kHistogramAttr);
        }

        Histogram(const Histogram &) = delete;

        Histogram &operator=(const Histogram &) = delete;

        ~Histogram() override = default;

        void set_boundaries(const std::array<T, N> &bins) {
            _bins = bins;
        }

        void set_boundaries(const std::array<T, N> &&bins) {
            _bins = bins;
        }

        const std::array<T, N> &get_boundaries() const {
            return _bins;
        }

        ScopeLatency<Histogram, ToMilliseconds> scope_latency_milliseconds() {
            return ScopeLatency<Histogram, ToMilliseconds>(this, ToMilliseconds());
        }

        ScopeLatency<Histogram, ToMicroseconds> scope_latency_microseconds() {
            return ScopeLatency<Histogram, ToMicroseconds>(this, ToMicroseconds());
        }

        ScopeLatency<Histogram, ToNanoseconds> scope_latency_nanoseconds() {
            return ScopeLatency<Histogram, ToNanoseconds>(this, ToNanoseconds());
        }

        ScopeLatency<Histogram, ToSeconds> scope_latency_seconds() {
            return ScopeLatency<Histogram, ToSeconds>(this, ToSeconds());
        }

        ScopeLatency<Histogram, ToDoubleSeconds> scope_latency_double_seconds() {
            return ScopeLatency<Histogram, ToDoubleSeconds>(this, ToDoubleSeconds());
        }

        ScopeLatency<Histogram, ToDoubleMilliseconds> scope_latency_double_milliseconds() {
            return ScopeLatency<Histogram, ToDoubleMilliseconds>(this, ToDoubleMilliseconds());
        }

        ScopeLatency<Histogram, ToDoubleMicroseconds> scope_latency_double_microseconds() {
            return ScopeLatency<Histogram, ToDoubleMicroseconds>(this, ToDoubleMicroseconds());
        }

        ScopeLatency<Histogram, ToDoubleNanoseconds> scope_latency_double_nanoseconds() {
            return ScopeLatency<Histogram, ToDoubleNanoseconds>(this, ToDoubleNanoseconds());
        }

        void get_value(HistogramResult<T> &result) const {
            result.bins.resize(N);
            result.boundaries.resize(N);
            result.count = 0;
            for (size_t i = 0; i < N; ++i) {
                result.bins[i] = _reducer.get_value(i);
                result.count += result.bins[i];
                result.boundaries[i] = _bins[i];
            }
            result.sum = _reducer.get_value(N);
            result.avg = result.sum / result.count;
        }

        HistogramResult<T> get_value() const {
            HistogramResult<T> result;
            get_value(result);
            return result;
        }


        Histogram &add_value(const T &value) {
            auto index = find_bin(value);
            if (index >= N) {
                return *this;
            }
            _reducer.set_value(1, index);
            _reducer.set_value(value, N);
            return *this;
        }

        Histogram &operator<<(const T &value) {
            return add_value(value);
        }
    private:
        std::string describe_impl(const DescriberOptions &options)  const override {
            return turbo::format("{}", get_value());
        }

        VariableSnapshot get_snapshot_impl() const override {
            using Htype = HistogramSnapshot;
            using Dtype = double;
            Htype snapshot;
            snapshot.name = name();
            snapshot.description = description();
            snapshot.labels = labels();
            snapshot.type = attr().type;
            snapshot.bins.resize(N);
            snapshot.boundaries.resize(N);

            for (size_t i = 0; i < N; ++i) {
                snapshot.bins[i] = static_cast<Dtype>(_reducer.get_value(i));
                snapshot.count += snapshot.bins[i];
                snapshot.boundaries[i] = _bins[i];
            }
            snapshot.sum = _reducer.get_value(N);
            snapshot.avg = snapshot.sum / snapshot.count;
            return snapshot;
        }
    private:
        size_t find_bin(const T &value) {
            return std::distance(_bins.begin(),
                                 std::find_if(_bins.begin(), _bins.end(), [value](const T &v) {
                                     return value < v;
                                 })
            );
        }

    private:
        std::array<T, N> _bins;
        reducer_type _reducer;
        turbo::Status _status;
    };

    template<typename T, typename Char>
    struct formatter<HistogramResult<T>, Char> : formatter<T, Char> {
        template<typename FormatContext>
        auto format(const HistogramResult<T> &t, FormatContext &ctx) {
            auto out = ctx.out();
            out = format_to(out, "HistogramResult:\n");
            out = format_to(out, "sum: {}\n", t.sum);
            out = format_to(out, "count: {}\n", t.count);
            out = format_to(out, "avg: {}\n", t.avg);
            out = format_to(out, "bin[{}]: ({}-{}): {}\n", 0, std::numeric_limits<T>::min(), t.boundaries[0],
                            t.bins[0]);
            for (size_t i = 1; i < t.boundaries.size(); ++i) {
                out = format_to(out, "bin[{}]: [{}-{}): {}\n", i, t.boundaries[i - 1], t.boundaries[i], t.bins[i]);

            }

            return out;
        }
    };

    template<typename T, size_t N, typename Char>
    struct formatter<Histogram<T, N>, Char, std::enable_if_t<is_atomical<T>::value>>
            : formatter<HistogramResult<T>, Char> {
        template<typename FormatContext>
        auto format(const Histogram<T, N> &t, FormatContext &ctx) {
            HistogramResult<T> result;
            t.get_value(result);
            return formatter<HistogramResult<T>, Char>::format(result, ctx);
        }
    };

}  // namespace turbo

#endif  // TURBO_PROFILING_HISTOGRAM_H_
