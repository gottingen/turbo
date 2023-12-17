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

namespace turbo {

    class Variable;

    struct VariableFilter {
        virtual ~VariableFilter() = default;

        /// @brief Filter the variable.
        /// @param variable The variable to filter.
        /// @return True if the variable should be included in the output.
        virtual bool filter(const Variable &variable) const = 0;
    };

    struct Describer{
        virtual ~Describer() = default;

        virtual std::string describe(const Variable *variable) const = 0;
    };

    struct Dumper {
        virtual ~Dumper() = default;

        virtual void dump(const Variable *variable) = 0;
    };

    class Variable {
    public:
        Variable() = default;

        virtual ~Variable();

        turbo::Status expose(const std::string_view &name, const std::string_view &description,
                             const std::map<std::string_view, std::string_view> &labels, const std::string_view &type);

        turbo::Status hide();

        [[nodiscard]] bool is_exposed() const;

        [[nodiscard]] const std::string &name() const {
            return name_;
        }

        [[nodiscard]] const std::string &description() const {
            return description_;
        }

        [[nodiscard]] const std::map<std::string, std::string> &labels() const {
            return labels_;
        }

        [[nodiscard]] const std::string &type() const {
            return type_;
        }

        static void list_exposed(std::vector<std::string> &names, const VariableFilter *filter = nullptr);

        static size_t count_exposed(const VariableFilter *filter = nullptr);
    public:

        template<typename D, typename ...Args>
        void describe(std::ostream &os, const D &d, Args &&...args) const {
            os << d(this, std::forward<Args>(args)...);
        }

        template<typename D, typename ...Args>
        [[nodiscard]] std::string describe(const D &d, Args &&...args) const {
            return d(this, std::forward<Args>(args)...);
        }


    private:
        virtual turbo::Status expose_impl(const std::string_view &name, const std::string_view &description,
                             const std::map<std::string_view, std::string_view> &labels, const std::string_view &type);
        // NOLINTNEXTLINE
        TURBO_NON_COPYABLE(Variable);
        template <typename H>
        friend H hash_value(H h, const Variable &c);
    private:
        std::string name_;
        std::string description_;
        std::map<std::string, std::string> labels_;
        std::string type_;
    };

    template <typename H>
    H hash_value(H h, const Variable &c) {
        return H::combine(std::move(h), c.name_, c.description_, c.labels_, c.type_);
    }

    template <typename Char>
    struct formatter<Variable, Char> : formatter<std::string_view, Char> {
        template <typename FormatContext>
        auto format(const Variable &c, FormatContext &ctx) -> decltype(ctx.out()) {
            //return formatter<std::string_view, Char>::format(c.describe(), ctx);
            return "Variable";
        }
    };
}  // namespace turbo

#endif  // TURBO_PROFILING_VARIABLE_H_
