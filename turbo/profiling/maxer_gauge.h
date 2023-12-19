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
// Created by jeff on 23-12-19.
//

#ifndef TURBO_PROFILING_MAXER_GAUGE_H_
#define TURBO_PROFILING_MAXER_GAUGE_H_

#include "turbo/profiling/variable.h"
#include "turbo/profiling/internal/reducer.h"
#include "turbo/profiling/internal/operators.h"

namespace turbo {

    template<typename T>
    class MaxerGauge : public Variable {
    public:
        MaxerGauge() :_status(unavailable_error("")) {}

        explicit MaxerGauge(const std::string_view &name, const std::string_view &description = "");

        MaxerGauge(const std::string_view &name, const std::string_view &description,
                     const std::map<std::string,std::string> &tags);

        MaxerGauge(const MaxerGauge &) = delete;

        MaxerGauge &operator=(const MaxerGauge &) = delete;

        ~MaxerGauge() override = default;

        void set(const T &value) {
            _reducer<<(value);
        }

        void operator=(const T &value) {
            _reducer<<(value);
        }

        MaxerGauge&operator<<(const T &value) {
            _reducer<<(value);
            return *this;
        }

        operator T() const {
            return _reducer.get_value();
        }

        void reset() {
            _reducer.reset();
        }

        T get_value() const {
            return _reducer.get_value();
        }

        bool valid() const {
            return _status.ok() && _reducer.valid();
        }

        [[nodiscard]] const turbo::Status &status() const {
            return _status;
        }

    private:
        std::string describe_impl(const DescriberOptions &options)  const override {
            return turbo::format("{}[{}-{}] : {}", name(), description(), labels(), get_value());
        }

    private:
        typedef profiling_internal::Reducer<T, profiling_internal::MaxerTo<T>,
                profiling_internal::SetTo<T> > reducer_type;
        reducer_type _reducer;
        turbo::Status _status;
    };

    template<typename T>
    MaxerGauge<T>::MaxerGauge(const std::string_view &name, const std::string_view &description)
            : Variable() {
        std::string desc(description);
        if(desc.empty()){
            desc = turbo::format("MaxerGuage-{}",  name);
        }
        _status = expose(name, desc, {}, "gauge");
    }

    template<typename T>
    MaxerGauge<T>::MaxerGauge(const std::string_view &name, const std::string_view &description,
                                  const std::map<std::string, std::string> &tags)
            : Variable() {
        _status = expose(name, description, tags, "gauge");
    }

    template<typename T, typename Char>
    struct formatter<MaxerGauge<T>, Char> : formatter<T, Char> {
        template<typename FormatContext>
        auto format(const MaxerGauge<T> &c, FormatContext &ctx) -> decltype(ctx.out()) {
            return formatter<T, Char>::format(c.get_value(), ctx);
        }
    };
}  // namespace turbo
#endif // TURBO_PROFILING_MAXER_GAUGE_H_
