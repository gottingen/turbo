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


#ifndef TURBO_PROFILING_UNIQUE_GAUGE_H_
#define TURBO_PROFILING_UNIQUE_GAUGE_H_

#include "turbo/profiling/variable.h"
#include "turbo/profiling/internal/reducer.h"
#include "turbo/profiling/internal/operators.h"
#include <numeric>
#include <limits>
#include <mutex>

namespace turbo {


    /**
     * @ingroup turbo_profiling_gauges
     * @brief UniqueGauge is a gauge that can be set to any value. the unique gauge is not aggregated.
     *        use this gauge to expose a value that can be set to any value. and it is not using the reducer
     *        for multiple threads values aggregation. the value is set to global value.
     *        eg: the number of threads in the thread pool, the cpu usage, the memory usage, the disk usage, etc.
     * @tparam T
     * @tparam E
     */
    template<typename T, typename E=void>
    class UniqueGauge : public Variable {
    public:
        static constexpr VariableAttr kUniqueOBjectGaugeAttr = VariableAttr(DUMP_PLAIN_TEXT, VariableType::VT_OBJECT);
    public:
        UniqueGauge() : _status(unavailable_error("")) {}

        explicit UniqueGauge(const std::string_view &name, const std::string_view &description = "");

        UniqueGauge(const std::string_view &name, const std::string_view &description,
                    const std::map<std::string, std::string> &tags);

        void set(const T &value) {
            std::unique_lock<std::mutex> lock(_mutex);
            _value = value;
        }

        void operator=(const T &value) {
            std::unique_lock<std::mutex> lock(_mutex);
            _value = value;
        }

        UniqueGauge &operator<<(const T &value) {
            std::unique_lock<std::mutex> lock(_mutex);
            _value = value;
            return *this;
        }

        T get_value() const {
            std::unique_lock<std::mutex> lock(_mutex);
            return _value;
        }

        operator T() const {
            std::unique_lock<std::mutex> lock(_mutex);
            return _value;
        }

        bool is_valid() const {
            return _status.ok();
        }

        const Status& status() const {
            return _status;
        }
    private:
        std::string describe_impl(const DescriberOptions &options)  const override {
            return turbo::format("{}[{}-{}] : {}", name(), description(), labels(), get_value());
        }

        VariableSnapshot get_snapshot_impl() const override {
            ObjectSnapshot snapshot;
            snapshot.name = name();
            snapshot.description = description();
            snapshot.labels = labels();
            snapshot.type = attr().type;
            T v = get_value();
            std::stringstream ss;
            ss<<v;
            snapshot.value = ss.str();
            snapshot.type_id = turbo::nameof_short_type<T>();
            return snapshot;
        }
    private:
        T _value;
        Status _status;
        mutable std::mutex _mutex;
    };

    template<typename T, typename E>
    UniqueGauge<T, E>::UniqueGauge(const std::string_view &name, const std::string_view &description)
            : Variable() {
        _status = expose(name, description, {}, kUniqueOBjectGaugeAttr);
    }

    template<typename T, typename E>
    UniqueGauge<T, E>::UniqueGauge(const std::string_view &name, const std::string_view &description,
                                   const std::map<std::string, std::string> &tags)
            : Variable() {
        _status = expose(name, description, tags, kUniqueOBjectGaugeAttr);
    }

    template<typename T>
    class UniqueGauge<T, typename std::enable_if_t<is_atomical<T>::value>> : public Variable {
    public:
        static constexpr VariableAttr kUniqueScalarGaugeAttr = VariableAttr(DUMP_PROMETHEUS_TYPE, VariableType::VT_GAUGE_SCALAR);
    public:
        UniqueGauge() : _status(unavailable_error("")) {}

        explicit UniqueGauge(const std::string_view &name, const std::string_view &description = "");

        UniqueGauge(const std::string_view &name, const std::string_view &description,
                    const std::map<std::string, std::string> &tags);

        void set(const T &value) {
            _value.store(value, std::memory_order_relaxed);
        }

        void operator=(const T &value) {
            set(value);
        }

        template<class U>
        UniqueGauge &operator<<(U value) {
            add(value);
            return *this;
        }

        void add(T n) {
            _value.fetch_add(n, std::memory_order_relaxed);
        }

        void sub(T n) {
            _value.fetch_sub(n, std::memory_order_relaxed);
        }

        T get_value() const {
            return _value.load(std::memory_order_relaxed);
        }

        operator T() const {
            return get_value();
        }

        bool is_valid() const {
            return _status.ok();
        }

        const Status& status() const {
            return _status;
        }
    private:
        std::string describe_impl(const DescriberOptions &options)  const override {
            return turbo::format("{}[{}-{}] : {}", name(), description(), labels(), get_value());
        }

        VariableSnapshot get_snapshot_impl() const override {
            using Gtype = GaugeSnapshot;
            using Dtype = double;
            Gtype snapshot;
            snapshot.value = static_cast<Dtype>(get_value());
            snapshot.name = name();
            snapshot.description = description();
            snapshot.labels = labels();
            snapshot.type = attr().type;
            return snapshot;
        }
    private:
        std::atomic<T> _value;
        Status _status;
    };

    template<typename T>
    UniqueGauge<T, typename std::enable_if_t<is_atomical<T>::value>>::UniqueGauge(const std::string_view &name,
                                                                                  const std::string_view &description)
            : Variable() {
        _status = expose(name, description, {}, kUniqueScalarGaugeAttr);
    }

    template<typename T>
    UniqueGauge<T, typename std::enable_if_t<is_atomical<T>::value>>::UniqueGauge(const std::string_view &name,
                                                                                  const std::string_view &description,
                                                                                  const std::map<std::string, std::string> &tags)
            : Variable() {
        _status = expose(name, description, tags, kUniqueScalarGaugeAttr);
    }

    template<>
    class UniqueGauge<std::string, void> : public Variable {
    public:
        static constexpr VariableAttr kUniqueStringGaugeAttr = VariableAttr(DUMP_PLAIN_TEXT, VariableType::VT_PLAIN_STRING);
    public:
        UniqueGauge() : _status(unavailable_error("")) {}

        explicit UniqueGauge(const std::string_view &name, const std::string_view &description = "");

        UniqueGauge(const std::string_view &name, const std::string_view &description,
                    const std::map<std::string, std::string> &tags);

        void set(const std::string &value) {
            std::unique_lock<std::mutex> lock(_mutex);
            _value = value;
        }

        void operator=(const std::string &value) {
            std::unique_lock<std::mutex> lock(_mutex);
            _value = value;
        }

        UniqueGauge &operator<<(const std::string &value) {
            std::unique_lock<std::mutex> lock(_mutex);
            _value = value;
            return *this;
        }

        std::string get_value() const {
            std::unique_lock<std::mutex> lock(_mutex);
            return _value;
        }

        operator std::string() const {
            std::unique_lock<std::mutex> lock(_mutex);
            return _value;
        }

        bool is_valid() const {
            return _status.ok();
        }

        const Status& status() const {
            return _status;
        }
    private:
        std::string describe_impl(const DescriberOptions &options)  const override {
            return turbo::format("{}[{}-{}] : {}", name(), description(), labels(), get_value());
        }

        VariableSnapshot get_snapshot_impl() const override {
            PlainStringSnapshot snapshot;
            snapshot.value = get_value();
            snapshot.name = name();
            snapshot.description = description();
            snapshot.labels = labels();
            snapshot.type = attr().type;
            return snapshot;
        }
    private:
        std::string _value;
        Status _status;
        mutable std::mutex _mutex;
    };

    inline UniqueGauge<std::string, void>::UniqueGauge(const std::string_view &name, const std::string_view &description)
            : Variable() {
        _status = expose(name, description, {}, kUniqueStringGaugeAttr);
    }

    inline UniqueGauge<std::string, void>::UniqueGauge(const std::string_view &name, const std::string_view &description,
                                                const std::map<std::string, std::string> &tags)
            : Variable() {
        _status = expose(name, description, tags, kUniqueStringGaugeAttr);
    }

    template<typename R>
    class UniqueGauge<std::function<R()>, void> : public Variable {
    public:
        struct _ObjectAttr {
            static constexpr VariableAttr value = VariableAttr(DUMP_PLAIN_TEXT, VariableType::VT_OBJECT);
        };

        struct _scalarAttr {
            static constexpr VariableAttr value = VariableAttr(DUMP_PROMETHEUS_TYPE, VariableType::VT_GAUGE_SCALAR);
        };
        static constexpr VariableAttr kUniqueFuncGauge = std::conditional<is_atomical<R>::value, _scalarAttr, _ObjectAttr>::type::value;
    public:
        UniqueGauge() : _status(unavailable_error("")) {}

        explicit UniqueGauge(const std::string_view &name, const std::string_view &description = "");

        UniqueGauge(const std::string_view &name, const std::string_view &description,
                    const std::map<std::string, std::string> &tags);

        void set(const std::function<R()> &value) {
            _value = value;
        }

       R get_value() const {
            return _value();
        }

        operator R() const {
            return _value();
        }

        bool is_valid() const {
            return _status.ok();
        }

        const Status& status() const {
            return _status;
        }
    private:
        std::string describe_impl(const DescriberOptions &options)  const override {
            return turbo::format("{}[{}-{}] : {}", name(), description(), labels(), get_value());
        }

        VariableSnapshot get_snapshot_impl() const override {
            using Gtype = typename std::conditional<is_atomical<R>::value,GaugeSnapshot, ObjectSnapshot>::type;
            Gtype snapshot;
            snapshot.name = name();
            snapshot.description = description();
            snapshot.labels = labels();
            snapshot.type = attr().type;
            R r = get_value();
            if constexpr (is_prometheus(kUniqueFuncGauge.type)) {
                snapshot.value = r;
            } else {
                std::stringstream ss;
                ss<<r;
                snapshot.value = ss.str();
                std::memcpy(snapshot.value.data(), &r, sizeof(R));
                snapshot.type_id = nameof_short_type<R>();
            }
            return snapshot;
        }

    private:
        std::function<R()> _value;
        Status _status;
    };

    template<typename R>
    UniqueGauge<std::function<R()>, void>::UniqueGauge(const std::string_view &name, const std::string_view &description)
            : Variable() {
        _status = expose(name, description, {}, kUniqueFuncGauge);
    }

    template<typename R>
    UniqueGauge<std::function<R()>, void>::UniqueGauge(const std::string_view &name, const std::string_view &description,
                                                const std::map<std::string, std::string> &tags)
            : Variable() {
        _status = expose(name, description, tags, kUniqueFuncGauge);
    }

    template<typename R, typename Char>
    struct formatter<UniqueGauge<std::function<R()>,void>, Char> : formatter<R, Char> {
        template<typename FormatContext>
        auto format(const UniqueGauge<std::function<R()>,void> &t, FormatContext &ctx) {
            return formatter<R, Char>::format(t.get_value(), ctx);
        }
    };
}  // namespace turbo

#endif  // TURBO_PROFILING_UNIQUE_GAUGE_H_
