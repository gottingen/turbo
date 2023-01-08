
#ifndef  FLARE_VARIABLE_WINDOW_H_
#define  FLARE_VARIABLE_WINDOW_H_

#include <limits>                                 // std::numeric_limits
#include <math.h>                                 // round
#include <gflags/gflags_declare.h>
#include "flare/log/logging.h"                         // FLARE_LOG
#include "flare/metrics/detail/sampler.h"
#include "flare/metrics/detail/series.h"
#include "flare/metrics/variable_base.h"

namespace flare {

    DECLARE_int32(variable_dump_interval);

    enum SeriesFrequency {
        SERIES_IN_WINDOW = 0,
        SERIES_IN_SECOND = 1
    };

    namespace metrics_detail {
        // Just for constructor reusing of window<>
        template<typename R, SeriesFrequency series_freq>
        class window_base : public variable_base {
        public:
            typedef typename R::value_type value_type;
            typedef typename R::sampler_type sampler_type;

            class series_sampler : public metrics_detail::variable_sampler {
            public:
                struct Op {
                    explicit Op(R *var) : _var(var) {}

                    void operator()(value_type &v1, const value_type &v2) const {
                        _var->op()(v1, v2);
                    }

                private:
                    R *_var;
                };

                series_sampler(window_base *owner, R *var)
                        : _owner(owner), _series(Op(var)) {}

                ~series_sampler() {}

                void take_sample() override {
                    if (series_freq == SERIES_IN_SECOND) {
                        // Get one-second window value for per_second<>, otherwise the
                        // "smoother" plot may hide peaks.
                        _series.append(_owner->get_value(1));
                    } else {
                        // Get the value inside the full window. "get_value(1)" is
                        // incorrect when users intend to see aggregated values of
                        // the full window in the plot.
                        _series.append(_owner->get_value());
                    }
                }

                void describe(std::ostream &os) { _series.describe(os, nullptr); }

            private:
                window_base *_owner;
                metrics_detail::series<value_type, Op> _series;
            };

            window_base(R *var, time_t window_size)
                    : _var(var), _window_size(window_size > 0 ? window_size : FLAGS_variable_dump_interval),
                      _sampler(var->get_sampler()), _series_sampler(nullptr) {
                FLARE_CHECK_EQ(0, _sampler->set_window_size(_window_size));
            }

            ~window_base() {
                hide();
                if (_series_sampler) {
                    _series_sampler->destroy();
                    _series_sampler = nullptr;
                }
            }

            bool get_span(time_t window_size, metrics_detail::variable_sample<value_type> *result) const {
                return _sampler->get_value(window_size, result);
            }

            bool get_span(metrics_detail::variable_sample<value_type> *result) const {
                return get_span(_window_size, result);
            }

            virtual value_type get_value(time_t window_size) const {
                metrics_detail::variable_sample<value_type> tmp;
                if (get_span(window_size, &tmp)) {
                    return tmp.data;
                }
                return value_type();
            }

            value_type get_value() const { return get_value(_window_size); }

            void describe(std::ostream &os, bool quote_string) const override {
                if (std::is_same<value_type, std::string>::value && quote_string) {
                    os << '"' << get_value() << '"';
                } else {
                    os << get_value();
                }
            }

            time_t window_size() const { return _window_size; }

            int describe_series(std::ostream &os, const variable_series_options &options) const override {
                if (_series_sampler == nullptr) {
                    return 1;
                }
                if (!options.test_only) {
                    _series_sampler->describe(os);
                }
                return 0;
            }

            void get_samples(std::vector<value_type> *samples) const {
                samples->clear();
                samples->reserve(_window_size);
                return _sampler->get_samples(samples, _window_size);
            }

        protected:

            int expose_impl(const std::string_view &prefix,
                            const std::string_view &name,
                            const std::string_view &help,
                            const std::unordered_map<std::string, std::string> &tags,
                            display_filter filter) override {
                const int rc = variable_base::expose_impl(prefix, name, help, tags, filter);
                if (rc == 0 &&
                    _series_sampler == nullptr &&
                    FLAGS_save_series) {
                    _series_sampler = new series_sampler(this, _var);
                    _series_sampler->schedule();
                }
                return rc;
            }

            R *_var;
            time_t _window_size;
            sampler_type *_sampler;
            series_sampler *_series_sampler;
        };

    }  // namespace detail

    // Get data within a time window.
    // The time unit is 1 second fixed.
    // window relies on other variable which should be constructed before this window
    // and destructs after this window.

    // R must:
    // - have get_sampler() (not require thread-safe)
    // - defined value_type and sampler_type
    template<typename R, SeriesFrequency series_freq = SERIES_IN_WINDOW>
    class window : public metrics_detail::window_base<R, series_freq> {
        typedef metrics_detail::window_base<R, series_freq> Base;
        typedef typename R::value_type value_type;
        typedef typename R::sampler_type sampler_type;
    public:
        // Different from per_second, we require window_size here because get_value
        // of window is largely affected by window_size while per_second is not.
        window(R *var, time_t window_size) : Base(var, window_size) {}

        window(const std::string_view &name, R *var, time_t window_size)
                : Base(var, window_size) {
            this->expose(name, "");
        }

        window(const std::string_view &prefix,
               const std::string_view &name, R *var, time_t window_size)
                : Base(var, window_size) {
            this->expose_as(prefix, name, "");
        }
    };

    // Get data per second within a time window.
    // The only difference between per_second and window is that per_second divides
    // the data by time duration.
    template<typename R>
    class per_second : public metrics_detail::window_base<R, SERIES_IN_SECOND> {
        typedef metrics_detail::window_base<R, SERIES_IN_SECOND> Base;
        typedef typename R::value_type value_type;
        typedef typename R::sampler_type sampler_type;
    public:
        // If window_size is non-positive or absent, use FLAGS_variable_dump_interval.
        per_second(R *var) : Base(var, -1) {}

        per_second(R *var, time_t window_size) : Base(var, window_size) {}

        per_second(const std::string_view &name, R *var) : Base(var, -1) {
            this->expose(name, "");
        }

        per_second(const std::string_view &name, R *var, time_t window_size)
                : Base(var, window_size) {
            this->expose(name, "");
        }

        per_second(const std::string_view &prefix,
                  const std::string_view &name, R *var)
                : Base(var, -1) {
            this->expose_as(prefix, name, "");
        }

        per_second(const std::string_view &prefix,
                  const std::string_view &name, R *var, time_t window_size)
                : Base(var, window_size) {
            this->expose_as(prefix, name);
        }

        value_type get_value(time_t window_size) const override {
            metrics_detail::variable_sample<value_type> s;
            this->get_span(window_size, &s);
            // We may test if the multiplication overflows and use integral ops
            // if possible. However signed/unsigned 32-bit/64-bit make the solution
            // complex. Since this function is not called often, we use floating
            // point for simplicity.
            if (s.time_us <= 0) {
                return static_cast<value_type>(0);
            }
            if (std::is_floating_point<value_type>::value) {
                return static_cast<value_type>(s.data * 1000000.0 / s.time_us);
            } else {
                return static_cast<value_type>(round(s.data * 1000000.0 / s.time_us));
            }
        }
    };

}  // namespace flare

#endif  // FLARE_VARIABLE_WINDOW_H_
