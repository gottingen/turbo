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


#ifndef  TURBO_VAR_STATUS_H_
#define  TURBO_VAR_STATUS_H_

#include <string>                       // std::string
#include <atomic>
#include <type_traits>
#include "turbo/var/variable.h"
#include "turbo/var/operators.h"

TURBO_DECLARE_FLAG(bool, var_save_series);

namespace turbo {

    template<typename T, typename Enabler = void>
    class StatusVar : public Variable {
    public:
        StatusVar() {}

        StatusVar(const T &value) : _value(value) {}

        StatusVar(const std::string &name, const T &value) : _value(value) {
            this->expose(name);
        }

        StatusVar(const std::string &prefix,
                  const std::string &name, const T &value) : _value(value) {
            this->expose_as(prefix, name);
        }

        // Calling hide() manually is a MUST required by Variable.
        ~StatusVar() { hide(); }

        void describe(std::ostream &os, bool /*quote_string*/) const override {
            os << get_value();
        }


        T get_value() const {
            std::unique_lock guard(_lock);
            const T res = _value;
            return res;
        }

        void set_value(const T &value) {
            std::unique_lock guard(_lock);
            _value = value;
        }

    private:
        T _value;
        // We use lock rather than std::atomic for generic values because
        // std::atomic requires the type to be memcpy-able (POD basically)
        mutable std::mutex _lock;
    };

    template<typename T>
    class StatusVar<T, typename std::enable_if<turbo::is_atomical<T>::value>::type>
            : public Variable {
    public:
        struct PlaceHolderOp {
            void operator()(T &, const T &) const {}
        };

        class SeriesSampler : public var_internal::Sampler {
        public:
            typedef typename std::conditional<
                    true, var_internal::AddTo<T>, PlaceHolderOp>::type Op;

            explicit SeriesSampler(StatusVar *owner)
                    : _owner(owner), _series(Op()) {}

            void take_sample() { _series.append(_owner->get_value()); }

            void describe(std::ostream &os) { _series.describe(os, nullptr); }

        private:
            StatusVar *_owner;
            var_internal::Series<T, Op> _series;
        };

    public:
        StatusVar() : _series_sampler(nullptr) {}

        StatusVar(const T &value) : _value(value), _series_sampler(nullptr) {}

        StatusVar(const std::string &name, const T &value)
                : _value(value), _series_sampler(nullptr) {
            this->expose(name);
        }

        StatusVar(const std::string &prefix,
                  const std::string &name, const T &value)
                : _value(value), _series_sampler(nullptr) {
            this->expose_as(prefix, name);
        }

        ~StatusVar() {
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

        int describe_series(std::ostream &os, const SeriesOptions &options) const override {
            if (_series_sampler == nullptr) {
                return 1;
            }
            if (!options.test_only) {
                _series_sampler->describe(os);
            }
            return 0;
        }

    protected:
        int expose_impl(const std::string_view &prefix,
                        const std::string_view &name,
                        DisplayFilter display_filter) override {
            const int rc = Variable::expose_impl(prefix, name, display_filter);
            if (rc == 0 &&
                _series_sampler == nullptr &&
                get_flag(FLAGS_var_save_series)) {
                _series_sampler = new SeriesSampler(this);
                _series_sampler->schedule();
            }
            return rc;
        }

    private:
        std::atomic<T> _value;
        SeriesSampler *_series_sampler;
    };

    template<>
    class StatusVar<std::string, void> : public Variable {
    public:
        StatusVar() {}

        template<typename ...Args>
        StatusVar(const std::string &name, std::string_view fmt, const Args &...args) {
            _value = turbo::format(fmt, args...);
            expose(name);
        }

        template<typename ...Args>
        StatusVar(const std::string &prefix,
                  const std::string_view &name, std::string_view fmt, const Args &...args) {
            _value = turbo::format(fmt, args...);
            expose_as(prefix, name);
        }

        ~StatusVar() { hide(); }

        void describe(std::ostream &os, bool quote_string) const override {
            if (quote_string) {
                os << '"' << get_value() << '"';
            } else {
                os << get_value();
            }
        }

        std::string get_value() const {
            std::unique_lock guard(_lock);
            return _value;
        }


        template<typename ...Args>
        void set_value(std::string_view fmt, const Args &...args) {
            _value = turbo::format(fmt, args...);
        }

        void set_value(const std::string &s) {
            std::unique_lock guard(_lock);
            _value = s;
        }

    private:
        std::string _value;
        mutable std::mutex _lock;
    };

}  // namespace turbo

#endif  // TURBO_VAR_STATUS_H_
