// Copyright 2013-2023 Daniel Parker
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

#ifndef JSONCONS_JSONSCHEMA_KEYWORD_VALIDATOR_HPP
#define JSONCONS_JSONSCHEMA_KEYWORD_VALIDATOR_HPP

#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/uri.h"
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/jsonpointer/jsonpointer.h"
#include "turbo/jsoncons/jsonschema/jsonschema_error.h"
#include "turbo/jsoncons/jsonschema/schema_location.h"

namespace turbo {
namespace jsonschema {

    // Interface for validation error handlers
    class error_reporter
    {
        bool fail_early_;
        std::size_t error_count_;
    public:
        error_reporter(bool fail_early = false)
            : fail_early_(fail_early), error_count_(0)
        {
        }

        virtual ~error_reporter() = default;

        void error(const validation_output& o)
        {
            ++error_count_;
            do_error(o);
        }

        std::size_t error_count() const
        {
            return error_count_;
        }

        bool fail_early() const
        {
            return fail_early_;
        }

    private:
        virtual void do_error(const validation_output& /* e */) = 0;
    };

    template <class Json>
    class keyword_validator 
    {
        std::string schema_path_;
    public:
        using validator_type = std::unique_ptr<keyword_validator<Json>>;
        using self_pointer = keyword_validator<Json>*;

        keyword_validator(const std::string& schema_path)
            : schema_path_(schema_path)
        {
        }

        keyword_validator(const keyword_validator&) = delete;
        keyword_validator(keyword_validator&&) = default;
        keyword_validator& operator=(const keyword_validator&) = delete;
        keyword_validator& operator=(keyword_validator&&) = default;

        virtual ~keyword_validator() = default;

        const std::string& schema_path() const
        {
            return schema_path_;
        }

        void validate(const Json& instance, 
                      const jsonpointer::json_pointer& instance_location, 
                      error_reporter& reporter, 
                      Json& patch) const 
        {
            do_validate(instance, 
                        instance_location,
                        reporter,
                        patch);
        }

        virtual std::optional<Json> get_default_value(const jsonpointer::json_pointer&, const Json&, error_reporter&) const
        {
            return std::optional<Json>();
        }

    private:
        virtual void do_validate(const Json& instance, 
                                 const jsonpointer::json_pointer& instance_location, 
                                 error_reporter& reporter, 
                                 Json& patch) const = 0;
    };

    template <class Json>
    using uri_resolver = std::function<Json(const turbo::uri & /*id*/)>;

    template <class Json>
    class reference_schema : public keyword_validator<Json>
    {
        using validator_type = typename std::unique_ptr<keyword_validator<Json>>;
        using validator_pointer = typename keyword_validator<Json>::self_pointer;

        validator_pointer referred_schema_;

    public:
        reference_schema(const std::string& id)
            : keyword_validator<Json>(id), referred_schema_(nullptr) {}

        void set_referred_schema(validator_pointer target) { referred_schema_ = target; }

    private:

        void do_validate(const Json& instance, 
                         const jsonpointer::json_pointer& instance_location, 
                         error_reporter& reporter, 
                         Json& patch) const override
        {
            if (!referred_schema_)
            {
                reporter.error(validation_output("", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Unresolved schema reference " + this->schema_path()));
                return;
            }

            referred_schema_->validate(instance, instance_location, reporter, patch);
        }

        std::optional<Json> get_default_value(const jsonpointer::json_pointer& instance_location,
                                                   const Json& instance, 
                                                   error_reporter& reporter) const override
        {
            if (!referred_schema_)
            {
                reporter.error(validation_output("", 
                                                 this->schema_path(), 
                                                 instance_location.to_uri_fragment(), 
                                                 "Unresolved schema reference " + this->schema_path()));
                return std::optional<Json>();
            }

            return referred_schema_->get_default_value(instance_location, instance, reporter);
        }
    };

} // namespace jsonschema
} // namespace turbo

#endif // JSONCONS_JSONSCHEMA_KEYWORD_VALIDATOR_HPP
