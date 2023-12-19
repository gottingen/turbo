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

#include "turbo/profiling/variable.h"
#include "turbo/container/flat_hash_map.h"
#include "turbo/log/logging.h"


namespace turbo {

    namespace profiling_internal {

        class VariableRegistry;

        class ScopedGuard {
        public:
            ~ScopedGuard();

        private:
            friend class VariableRegistry;

            ScopedGuard(VariableRegistry *registry);

        public:
            void create_variable(Variable *variable);

            void remove_variable(const std::string &name);

            Variable *get_variable(const std::string &name);

            const turbo::flat_hash_map<std::string, Variable *> &variables() const;

        private:
            VariableRegistry *registry_;
        };

        class VariableRegistry {
        public:
            ~VariableRegistry() = default;

            static VariableRegistry &get_instance() {
                static VariableRegistry instance;
                return instance;
            }

            ScopedGuard get_guard() {
                return ScopedGuard{this};
            }

        private:
            Variable *get_variable(const std::string &name) {
                auto it = variables_.find(name);
                if (it == variables_.end()) {
                    return nullptr;
                }
                return it->second;
            }

            void create_variable(Variable *variable) {
                auto it = variables_.find(variable->name());
                if (it != variables_.end()) {
                    return;
                }
                variables_.emplace(variable->name(), variable);
            }

            void remove_variable(const std::string &name) {
                variables_.erase(name);
            }

            [[nodiscard]] const turbo::flat_hash_map<std::string, Variable *> &variables() const {
                return variables_;
            }

        private:
            VariableRegistry() = default;

        private:
            friend class ScopedGuard;

            std::mutex mutex_;
            turbo::flat_hash_map<std::string, Variable *> variables_;
        };

        ScopedGuard::ScopedGuard(VariableRegistry *registry) : registry_(registry) {
            registry_->mutex_.lock();
        }

        ScopedGuard::~ScopedGuard() {
            registry_->mutex_.unlock();
        }

        void ScopedGuard::create_variable(Variable *variable) {
            registry_->create_variable(variable);
        }


        void ScopedGuard::remove_variable(const std::string &name) {
            registry_->remove_variable(name);
        }

        Variable *ScopedGuard::get_variable(const std::string &name) {
            return registry_->get_variable(name);
        }

        const turbo::flat_hash_map<std::string, Variable *> &ScopedGuard::variables() const {
            return registry_->variables();
        }

    }  // namespace profiling_internal
    Variable::~Variable() {
        auto rs = hide();
        TLOG_CHECK((rs.ok() || is_not_found(rs)), "Failed to hide variable :{}: {}", name_, rs.to_string());
    }
    turbo::Status Variable::expose(const std::string_view &name, const std::string_view &description,
                                   const std::map<std::string, std::string> &labels, const std::string_view &type) {
        return expose_impl(name, description, labels, type);
    }

    turbo::Status Variable::expose_impl(const std::string_view &name, const std::string_view &description,
                                        const std::map<std::string, std::string> &labels,
                                        const std::string_view &type) {
        if (is_exposed()) {
            return turbo::already_exists_error("Variable :{} is already exposed", name_);
        }
        if (name.empty()) {
            return turbo::invalid_argument_error("Variable name cannot be empty");
        }
        name_ = name;
        description_ = description;
        for (auto &label: labels) {
            labels_[std::string(label.first)] = std::string(label.second);
        }
        type_ = type;
        {
            auto guard = profiling_internal::VariableRegistry::get_instance().get_guard();
            guard.create_variable(this);
        }
        return turbo::ok_status();
    }

    turbo::Status Variable::hide() {
        if (!is_exposed()) {
            return turbo::not_found_error("Variable :{} is not exposed", name_);
        }
        {
            auto guard = profiling_internal::VariableRegistry::get_instance().get_guard();
            guard.remove_variable(name_);
        }
        name_.clear();
        description_.clear();
        labels_.clear();
        type_.clear();
        return turbo::ok_status();
    }

    bool Variable::is_exposed() const {
        return !name_.empty();
    }

    void Variable::list_exposed(std::vector<std::string> &names, const VariableFilter *filter) {
        auto guard = profiling_internal::VariableRegistry::get_instance().get_guard();
        if (filter == nullptr) {
            for (const auto &pair: guard.variables()) {
                names.emplace_back(pair.first);
            }
            return;
        }
        for (const auto &pair: guard.variables()) {
            if (filter->filter(*pair.second)) {
                names.emplace_back(pair.first);
            }
        }
    }

    size_t Variable::count_exposed(const VariableFilter *filter) {
        auto guard = profiling_internal::VariableRegistry::get_instance().get_guard();
        if (filter == nullptr) {
            return guard.variables().size();
        }
        size_t count = 0;
        for (const auto &pair: guard.variables()) {
            if (filter->filter(*pair.second)) {
                ++count;
            }
        }
        return count;

    }
}  // namespace turbo
