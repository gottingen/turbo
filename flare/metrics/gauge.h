
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_METRICS_GAUGE_H_
#define FLARE_METRICS_GAUGE_H_

#include <atomic>
#include <type_traits>
#include <string>
#include <mutex>
#include "flare/metrics/variable_reducer.h"
#include "flare/base/static_atomic.h"
#include "flare/base/type_traits.h"
#include "flare/strings/str_format.h"
#include "flare/metrics/detail/is_atomical.h"
#include "flare/metrics/variable_base.h"

namespace flare {
    namespace metrics_detail {

        struct add_filter {
            display_filter operator()(display_filter df) {
                return static_cast<display_filter>(df | DISPLAY_ON_METRICS);
            }
        };

        struct remove_filter {
            display_filter operator()(display_filter df) {
                return static_cast<display_filter>(df & ~DISPLAY_ON_METRICS);
            }
        };

        template<typename T>
        struct place_holder_collect {
            void operator()(const T *, cache_metrics &metric) const {}
        };

        template<typename T>
        struct metrics_collect {
            void operator()(const T *sg, cache_metrics &metric) const {
                sg->copy_metric_family(metric);
                metric.type = metrics_type::mt_gauge;
                metric.gauge.value = sg->get_value();
            }
        };
    }  // namespace metrics_detail

    template<typename T>
    class gauge : public variable_reducer<T, metrics_detail::add_to<T>, metrics_detail::minus_from<T> > {
    public:
        typedef variable_reducer<T, metrics_detail::add_to<T>, metrics_detail::minus_from<T>> Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;

        static const bool GAUGE_ABLE = (std::is_integral<T>::value ||
                                        std::is_floating_point<T>::value);

        typedef typename std::conditional<
                GAUGE_ABLE, metrics_detail::metrics_collect<gauge<T>>, metrics_detail::place_holder_collect<gauge<T>>>::type Co;
        typedef typename std::conditional<
                GAUGE_ABLE, metrics_detail::add_filter, metrics_detail::remove_filter>::type Filter;

    public:
        gauge() : Base() {}

        explicit gauge(const std::string_view &name,
                       const std::string_view &help = "",
                       const variable_base::tag_type &tags = variable_base::tag_type(),
                       display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)) : Base() {
            this->expose(name, help, tags, Filter()(f));
        }

        gauge(const std::string_view &prefix,
              const std::string_view &name,
              const std::string_view &help = "",
              const variable_base::tag_type &tags = variable_base::tag_type(),
              display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)) : Base() {
            this->expose_as(prefix, name, help, tags, Filter()(f));
        }

        void collect_metrics(cache_metrics &metric) const override {
            static Co c;
            c(this, metric);
        }

        ~gauge() { variable_base::hide(); }

    };

    template<typename T>
    class max_gauge : public variable_reducer<T, metrics_detail::max_to<T> > {
    public:
        typedef variable_reducer<T, metrics_detail::max_to<T>> Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        max_gauge() : Base(std::numeric_limits<T>::min()) {}

        explicit max_gauge(const std::string_view &name,
                           const std::string_view &help = "",
                           const variable_base::tag_type &tags = variable_base::tag_type(),
                           display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : Base(std::numeric_limits<T>::min()) {
            this->expose(name, help, tags, f);
        }

        max_gauge(const std::string_view &prefix, const std::string_view &name,
                  const std::string_view &help = "",
                  const variable_base::tag_type &tags = variable_base::tag_type(),
                  display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : Base(std::numeric_limits<T>::min()) {
            this->expose_as(prefix, name, help, tags, f);
        }

        ~max_gauge() { variable_base::hide(); }

        void collect_metrics(cache_metrics &metric) const override {
            this->copy_metric_family(metric);
            metric.type = metrics_type::mt_gauge;
            metric.counter.value = this->get_value();
        }

    private:

        friend class metrics_detail::LatencyRecorderBase;

        // The following private funcition a now used in LatencyRecorder,
        // it's dangerous so we don't make them public
        explicit max_gauge(T default_value) : Base(default_value) {
        }

        max_gauge(T default_value, const std::string_view &prefix,
                  const std::string_view &name,
                  const std::string_view &help = "",
                  const variable_base::tag_type &tags = variable_base::tag_type(),
                  display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)
        )
                : Base(default_value) {
            this->expose_as(prefix, name, help, tags, f);
        }

        max_gauge(T default_value, const std::string_view &name) : Base(default_value) {
            this->expose(name);
        }
    };

    template<typename T>
    class min_gauge : public variable_reducer<T, metrics_detail::min_to<T> > {
    public:
        typedef variable_reducer<T, metrics_detail::min_to<T>> Base;
        typedef T value_type;
        typedef typename Base::sampler_type sampler_type;
    public:
        min_gauge() : Base(std::numeric_limits<T>::max()) {}

        explicit min_gauge(const std::string_view &name,
                           const std::string_view &help = "",
                           const variable_base::tag_type &tags = variable_base::tag_type(),
                           display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : Base(std::numeric_limits<T>::min()) {
            this->expose(name, help, tags, f);
        }

        min_gauge(const std::string_view &prefix, const std::string_view &name,
                  const std::string_view &help = "",
                  const variable_base::tag_type &tags = variable_base::tag_type(),
                  display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : Base(std::numeric_limits<T>::min()) {
            this->expose_as(prefix, name, help, tags, f);
        }

        ~min_gauge() { variable_base::hide(); }

        void collect_metrics(cache_metrics &metric) const override {
            this->copy_metric_family(metric);
            metric.type = metrics_type::mt_gauge;
            metric.counter.value = this->get_value();
        }

    private:

        friend class metrics_detail::LatencyRecorderBase;

        // The following private funcition a now used in LatencyRecorder,
        // it's dangerous so we don't make them public
        explicit min_gauge(T default_value) : Base(default_value) {
        }

        min_gauge(T default_value, const std::string_view &prefix,
                  const std::string_view &name,
                  const std::string_view &help = "",
                  const variable_base::tag_type &tags = variable_base::tag_type(),
                  display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)
        )
                : Base(default_value) {
            this->expose_as(prefix, name, help, tags, f);
        }

        min_gauge(T default_value, const std::string_view &name) : Base(default_value) {
            this->expose(name);
        }
    };

    template<typename Tp>
    class status_gauge : public variable_base {
    public:
        typedef Tp value_type;
        typedef metrics_detail::reducer_sampler<status_gauge, Tp, metrics_detail::add_to<Tp>,
                metrics_detail::minus_from<Tp> > sampler_type;

        struct place_holder_op {
            void operator()(Tp &, const Tp &) const {}
        };

        static const bool ADDITIVE = (std::is_integral<Tp>::value ||
                                      std::is_floating_point<Tp>::value ||
                                      is_vector<Tp>::value);

        static const bool GAUGE_ABLE = (std::is_integral<Tp>::value ||
                                        std::is_floating_point<Tp>::value);

        typedef typename std::conditional<
                GAUGE_ABLE, metrics_detail::metrics_collect<status_gauge<Tp>>, metrics_detail::place_holder_collect<status_gauge<Tp>>>::type Co;
        typedef typename std::conditional<
                GAUGE_ABLE, metrics_detail::add_filter, metrics_detail::remove_filter>::type Filter;

        class series_sampler : public metrics_detail::variable_sampler {
        public:
            typedef typename std::conditional<
                    ADDITIVE, metrics_detail::add_to<Tp>, place_holder_op>::type Op;


            explicit series_sampler(status_gauge *owner)
                    : _owner(owner), _vector_names(nullptr), _series(Op()) {}

            ~series_sampler() {
                delete _vector_names;
            }

            void take_sample() override { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, _vector_names); }

            void set_vector_names(const std::string &names) {
                if (_vector_names == nullptr) {
                    _vector_names = new std::string;
                }
                *_vector_names = names;
            }

        private:
            status_gauge *_owner;
            std::string *_vector_names;
            metrics_detail::series<Tp, Op> _series;
        };

    public:
        // NOTE: You must be very careful about lifetime of `arg' which should be
        // valid during lifetime of status_gauge.
        status_gauge(const std::string_view &name,
                     Tp (*getfn)(void *), void *arg,
                     const std::string_view &help = "",
                     const variable_base::tag_type &tags = variable_base::tag_type(),
                     display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : _getfn(getfn), _arg(arg), _sampler(nullptr), _series_sampler(nullptr) {
            expose(name, help, tags, Filter()(f));
        }

        status_gauge(const std::string_view &prefix,
                     const std::string_view &name,
                     Tp (*getfn)(void *), void *arg,
                     const std::string_view &help = "",
                     const variable_base::tag_type &tags = variable_base::tag_type(),
                     display_filter f = static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS))
                : _getfn(getfn), _arg(arg), _sampler(nullptr), _series_sampler(nullptr) {
            expose_as(prefix, name, help, tags, Filter()(f));
        }

        status_gauge(Tp (*getfn)(void *), void *arg)
                : _getfn(getfn), _arg(arg), _sampler(nullptr), _series_sampler(nullptr) {
        }

        ~status_gauge() {
            hide();
            if (_sampler) {
                _sampler->destroy();
                _sampler = nullptr;
            }
            if (_series_sampler) {
                _series_sampler->destroy();
                _series_sampler = nullptr;
            }
        }

        int set_vector_names(const std::string &names) {
            if (_series_sampler) {
                _series_sampler->set_vector_names(names);
                return 0;
            }
            return -1;
        }

        void collect_metrics(cache_metrics &metric) const override {
            static Co c;
            c(this, metric);
        }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }

        Tp get_value() const {
            return (_getfn ? _getfn(_arg) : Tp());
        }

        sampler_type *get_sampler() {
            if (nullptr == _sampler) {
                _sampler = new sampler_type(this);
                _sampler->schedule();
            }
            return _sampler;
        }

        metrics_detail::add_to<Tp> op() const { return metrics_detail::add_to<Tp>(); }

        metrics_detail::minus_from<Tp> inv_op() const { return metrics_detail::minus_from<Tp>(); }

        int describe_series(std::ostream &os, const variable_series_options &options) const override {
            if (_series_sampler == nullptr) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

        Tp reset() {
            FLARE_CHECK(false) << "status_gauge::reset() should never be called, abort";
            abort();
        }

    protected:

        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        const std::string_view &help,
                        const std::unordered_map<std::string, std::string> &tags,
                        display_filter filter) override {
            const int rc = variable_base::expose_impl(prefix, name, help, tags, filter);
            if (ADDITIVE &&
                rc == 0 &&
                _series_sampler == nullptr &&
                FLAGS_save_series) {
                _series_sampler = new series_sampler(this);
                _series_sampler->schedule();
            }
            return rc;
        }

    private:

        Tp (*_getfn)(void *);

        void *_arg;
        sampler_type *_sampler;
        series_sampler *_series_sampler;
    };

    template<typename Tp> const bool status_gauge<Tp>::ADDITIVE;


// Specialize std::string for using std::ostream& as a more friendly
// interface for user's callback.
    template<>
    class status_gauge<std::string> : public variable_base {
    public:
        // NOTE: You must be very careful about lifetime of `arg' which should be
        // valid during lifetime of status_gauge.
        status_gauge(const std::string_view &name,
                     void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {
            expose(name, "");
        }

        status_gauge(const std::string_view &prefix,
                     const std::string_view &name,
                     void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {
            expose_as(prefix, name, "");
        }

        status_gauge(void (*print)(std::ostream &, void *), void *arg)
                : _print(print), _arg(arg) {}

        ~status_gauge() {
            hide();
        }

        void describe(std::ostream &os, bool quote_string) const override {
            if (quote_string) {
                if (_print) {
                    os << '"';
                    _print(os, _arg);
                    os << '"';
                } else {
                    os << "\"null\"";
                }
            } else {
                if (_print) {
                    _print(os, _arg);
                } else {
                    os << "null";
                }
            }
        }

    private:

        void (*_print)(std::ostream &, void *);

        void *_arg;
    };

    template<typename Tp>
    class basic_status_gauge : public status_gauge<Tp> {
    public:
        basic_status_gauge(const std::string_view &name,
                           Tp (*getfn)(void *), void *arg)
                : status_gauge<Tp>(name, getfn, arg) {}

        basic_status_gauge(const std::string_view &prefix,
                           const std::string_view &name,
                           Tp (*getfn)(void *), void *arg)
                : status_gauge<Tp>(prefix, name, getfn, arg) {}

        basic_status_gauge(Tp (*getfn)(void *), void *arg)
                : status_gauge<Tp>(getfn, arg) {}
    };

    template<>
    class basic_status_gauge<std::string> : public status_gauge<std::string> {
    public:
        basic_status_gauge(const std::string_view &name,
                           void (*print)(std::ostream &, void *), void *arg)
                : status_gauge<std::string>(name, print, arg) {}

        basic_status_gauge(const std::string_view &prefix,
                           const std::string_view &name,
                           void (*print)(std::ostream &, void *), void *arg)
                : status_gauge<std::string>(prefix, name, print, arg) {}

        basic_status_gauge(void (*print)(std::ostream &, void *), void *arg)
                : status_gauge<std::string>(print, arg) {}
    };


    template<typename T, typename Enabler = void>
    class read_most_gauge : public variable_base {
    public:
        read_most_gauge() {}

        read_most_gauge(const T &value) : _value(value) {}

        read_most_gauge(const std::string_view &name, const T &value, const std::string_view &help = "",
                        const variable_base::tag_type &tags = variable_base::tag_type()) : _value(
                value) {
            this->expose(name, help, tags, DISPLAY_ON_ALL);
        }

        read_most_gauge(const std::string_view &prefix,
                        const std::string_view &name, const T &value, const std::string_view &help = "",
                        const variable_base::tag_type &tags = variable_base::tag_type()) : _value(value) {
            this->expose_as(prefix, name, help, tags, DISPLAY_ON_ALL);
        }

        // Calling hide() manually is a MUST required by variable_base.
        ~read_most_gauge() { hide(); }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }

        T get_value() const {
            std::unique_lock<std::mutex> guard(_lock);
            const T res = _value;
            return res;
        }

        void set_value(const T &value) {
            std::unique_lock<std::mutex> guard(_lock);
            _value = value;
        }

    private:
        T _value;
        mutable std::mutex _lock;
    };

    template<typename T>
    class read_most_gauge<T, typename std::enable_if<metrics_detail::is_atomical<T>::value>::type>
            : public variable_base {
    public:
        struct place_holder_op {
            void operator()(T &, const T &) const {}
        };

        static const bool GAUGE_ABLE = (std::is_integral<T>::value ||
                                        std::is_floating_point<T>::value);

        typedef typename std::conditional<
                GAUGE_ABLE, metrics_detail::metrics_collect<read_most_gauge<T>>, metrics_detail::place_holder_collect<read_most_gauge<T>>>::type Co;
        typedef typename std::conditional<
                GAUGE_ABLE, metrics_detail::add_filter, metrics_detail::remove_filter>::type Filter;

        class series_sampler : public metrics_detail::variable_sampler {
        public:
            typedef typename std::conditional<
                    true, metrics_detail::add_to<T>, place_holder_op>
            ::type Op;

            explicit series_sampler(read_most_gauge *owner)
                    : _owner(owner), _series(Op()) {}

            void take_sample() { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, nullptr); }

        private:
            read_most_gauge *_owner;
            metrics_detail::series<T, Op> _series;
        };

    public:
        read_most_gauge() : _series_sampler(nullptr) {}

        read_most_gauge(const T &value) : _value(value), _series_sampler(nullptr) {}

        read_most_gauge(const std::string_view &name, const T &value, const std::string_view &help = "",
                        const variable_base::tag_type &tags = variable_base::tag_type())
                : _value(value), _series_sampler(nullptr) {
            this->expose(name, help, tags,
                         Filter()(static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)));
        }

        read_most_gauge(const std::string_view &prefix,
                        const std::string_view &name, const T &value, const std::string_view &help = "",
                        const variable_base::tag_type &tags = variable_base::tag_type())
                : _value(value), _series_sampler(nullptr) {
            this->expose_as(prefix, name, help, tags, Filter()(
                    static_cast<display_filter>(DISPLAY_ON_ALL | DISPLAY_ON_METRICS)));
        }

        ~read_most_gauge() {
            hide();
            if (_series_sampler) {
                _series_sampler->destroy();
                _series_sampler = nullptr;
            }
        }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }

        T get_value() const {
            return _value.load(std::memory_order_relaxed);
        }

        void set_value(const T &value) {
            _value.store(value, std::memory_order_relaxed);
        }

        int describe_series(std::ostream &os, const variable_series_options &options) const override {
            if (_series_sampler == nullptr) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

        void collect_metrics(cache_metrics &metric) const override {
            static Co c;
            c(this, metric);
        }

    protected:

        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        const std::string_view &help,
                        const std::unordered_map<std::string, std::string> &tags,
                        display_filter df) override {
            const int rc = variable_base::expose_impl(prefix, name, help, tags, Filter()(df));
            if (rc == 0 &&
                _series_sampler == nullptr &&
                FLAGS_save_series) {
                _series_sampler = new series_sampler(this);
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        std::atomic<T> _value;
        series_sampler *_series_sampler;
    };

    // Specialize for std::string, adding a printf-style set_value().
    template<>
    class read_most_gauge<std::string, void> : public variable_base {
    public:
        read_most_gauge() {}

        template<class... Args>
        read_most_gauge(const std::string_view &name, const std::string_view &fmt, Args &&... args) {
            _value = flare::string_format(fmt, std::forward<Args>(args)...);
            expose(name, "");
        }

        template<class... Args>
        read_most_gauge(const std::string_view &prefix,
                        const std::string_view &name, const std::string_view &fmt, Args &&... args) {
            _value = flare::string_format(fmt, std::forward<Args>(args)...);
            expose_as(prefix, name, "");
        }

        ~read_most_gauge() { hide(); }

        void describe(std::ostream &os, bool quote_string) const override {
            if (quote_string) {
                os << '"' << get_value() << '"';
            } else {
                os << get_value();
            }
        }

        std::string get_value() const {
            std::unique_lock<std::mutex> guard(_lock);
            return _value;
        }

        template<class... Args>
        void set_value(const std::string_view &fmt, Args &&... args) {
            std::unique_lock<std::mutex> guard(_lock);
            _value = flare::string_format(fmt, std::forward<Args>(args)...);
        }

        void set_value(const std::string &s) {
            std::unique_lock<std::mutex> guard(_lock);
            _value = s;
        }

    private:
        std::string _value;
        mutable std::mutex _lock;
    };


}  // namespace flare

#endif  // FLARE_METRICS_GAUGE_H_
