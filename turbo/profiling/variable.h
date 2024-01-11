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
// Created by jeff on 23-12-16.
//

#ifndef TURBO_PROFILING_VARIABLE_H_
#define TURBO_PROFILING_VARIABLE_H_

#include <map>
#include <string>
#include <vector>
#include <string_view>
#include "turbo/base/status.h"
#include "turbo/hash/hash.h"
#include "turbo/profiling/snapshot.h"
#include "turbo/profiling/dumper.h"
#include "turbo/profiling/prometheus_dumper.h"

namespace turbo {

    class Variable;

    struct VariableFilter {
        virtual ~VariableFilter() = default;

        /// @brief Filter the variable.
        /// @param variable The variable to filter.
        /// @return True if the variable should be included in the output.
        virtual bool filter(const Variable &variable) const = 0;
    };

    struct DescriberOptions {
        bool show_name = true;
        bool show_description = true;
        bool show_labels = true;
        bool show_type = true;
    };

    struct VariableAttr {
        constexpr VariableAttr() = default;
        constexpr VariableAttr(DumperType dumper_type, VariableType vt) : dumper_type(dumper_type), type(vt) {}

        DumperType dumper_type{DUMP_PROMETHEUS_TYPE};
        VariableType type{VariableType::VT_PROMETHEUS};

        uint8_t reserved[6]{0};
    };

    /**
     * @ingroup turbo_profiling_variable
     * @brief   ``Variable`` is the base class for all variables.
     *          It provides the basic functionality for exposing and hiding variables. The Variable  is
     *          Uniqued by name in global scope.
     *          the Variable is designed to hold variables that are written many threads and read by one thread.
     *          Most of time, the Variable is used to hold the metrics of the system.
     *          all the Metric type like Counter, Gauge, UniqueGauge, MinerGauge, AverageGauge are derived from Variable.
     *          The Variable is thread-safe. derived class should be implemented the virtual function describe_impl.
     *          more detail see the derived class, like Counter, Gauge, UniqueGauge, MinerGauge, AverageGauge.
     *          the main purpose of Variable is to provide a unified interface for all the metrics ,and when using the
     *          Variable, it cost little performance. for example, we want to record the number of the request. we can
     *          use the Counter like this:
     *          @code
     *          Counter<int> request_counter("request_counter","the number of the request");
     *          request_counter++;
     *          @endcode
     *          and we can get the value of the request_counter like this:
     *          @code
     *          int request_count = request_counter.get_value();
     *          @endcode
     *          it will cost little when call it because erery thread has a local copy of the request_counter. and when
     *          write , it only atomicly write it to the local copy. and when read, the combine will combine all the thread
     *          local copy to the global copy. so it cost little performance.
     *          so that do not read the value of the Variable frequently.
     */
    class Variable {
    public:
        static PrometheusDumper g_dumper;
    public:
        Variable() = default;

        virtual ~Variable();

        /**
         * @brief expose to the global scope.
         * @param name
         * @param description
         * @param labels
         * @param type
         * @return
         */
        turbo::Status expose_base(const std::string_view &name, const std::string_view &description,
                             const std::map<std::string, std::string> &labels, const VariableAttr &type);

        /**
         * @brief hide from the global scope.
         * @return
         */
        turbo::Status hide();

        /**
         * @brief check if the variable is exposed.
         * @return
         */
        [[nodiscard]] bool is_exposed() const;

        /**
         * @brief get the name of the variable.
         * @return
         */
        [[nodiscard]] const std::string &name() const {
            return name_;
        }

        /**
         * @brief get the description of the variable.
         * @return
         */
        [[nodiscard]] const std::string &description() const {
            return description_;
        }

        /**
         * @brief get the labels of the variable.
         * @return
         */
        [[nodiscard]] const std::map<std::string, std::string> &labels() const {
            return labels_;
        }

        /**
         * @brief get the attr of the variable.
         * @return
         */
        [[nodiscard]] const VariableAttr &attr() const {
            return _attr;
        }

        /**
         * @brief list all the exposed variable names.
         * @param names
         * @param filter
         */
        static void list_exposed(std::vector<std::string> &names, const VariableFilter *filter = nullptr);

        /**
         * @brief count exposed variable number.
         * @param filter
         * @return
         */
        static size_t count_exposed(const VariableFilter *filter = nullptr);

    public:

        /**
         * @brief describe variable to stream, for debug
         * @return
         */
        void describe(std::ostream &os, const DescriberOptions &options = DescriberOptions()) const {
            os << describe(options);
        }

        /**
         * @brief describe variable to stream, for debug
         * @return
         */
        [[nodiscard]] std::string describe(const DescriberOptions &options = DescriberOptions()) const {
            return describe_impl(options);
        }

        /**
         * @brief get the snapshot of the variable.
         * @return
         */
        [[nodiscard]] VariableSnapshot get_snapshot() const {
            return get_snapshot_impl();
        }

        /**
         * @brief dump the exposed Variable to the ostream.
         *        the format is prometheus text format.
         *        if the variable do not support prometheus, it will
         *        be return a string "unsupport".
         */
        [[nodiscard]] std::string dump_prometheus() const{
            return g_dumper.dump(get_snapshot());
        }

        /**
         * @brief dump all the exposed Variable to the ostream.
         *        the format is prometheus text format.
         *        if the variable do not support prometheus, it will
         *        be ignored.
         */
        static void dump_prometheus_all(std::ostream &os);

        static std::string dump_prometheus_all() {
            std::stringstream ss;
            dump_prometheus_all(ss);
            return ss.str();
        }

    private:
        virtual turbo::Status expose_impl(const std::string_view &name, const std::string_view &description,
                                          const std::map<std::string, std::string> &labels,
                                          const VariableAttr &type);

        virtual std::string describe_impl(const DescriberOptions &options) const = 0;

        virtual VariableSnapshot get_snapshot_impl() const = 0;

        // NOLINTNEXTLINE
        TURBO_NON_COPYABLE(Variable);

    private:
        std::string name_;
        std::string description_;
        std::map<std::string, std::string> labels_;
        VariableAttr _attr;
    };
}  // namespace turbo

#endif  // TURBO_PROFILING_VARIABLE_H_
