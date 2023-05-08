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

#ifndef JSONCONS_JSONSCHEMA_JSON_VALIDATOR_HPP
#define JSONCONS_JSONSCHEMA_JSON_VALIDATOR_HPP

#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/uri.h"
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/jsonpointer/jsonpointer.h"
#include "turbo/jsoncons/jsonschema/schema.h"
#include <cassert>
#include <set>
#include <sstream>
#include <iostream>
#include <cassert>
#include <functional>

namespace turbo {
namespace jsonschema {

    class throwing_error_reporter : public error_reporter
    {
        void do_error(const validation_output& o) override
        {
            JSONCONS_THROW(validation_error(o.message()));
        }
    };

    class fail_early_reporter : public error_reporter
    {
        void do_error(const validation_output&) override
        {
        }
    public:
        fail_early_reporter()
            : error_reporter(true)
        {
        }
    };

    using error_reporter_t = std::function<void(const validation_output& o)>;

    struct error_reporter_adaptor : public error_reporter
    {
        error_reporter_t reporter_;

        error_reporter_adaptor(const error_reporter_t& reporter)
            : reporter_(reporter)
        {
        }
    private:
        void do_error(const validation_output& e) override
        {
            reporter_(e);
        }
    };

    template <class Json>
    class json_validator
    {
        std::shared_ptr<json_schema<Json>> root_;

    public:
        json_validator(std::shared_ptr<json_schema<Json>> root)
            : root_(root)
        {
        }

        json_validator(json_validator &&) = default;
        json_validator &operator=(json_validator &&) = default;

        json_validator(json_validator const &) = delete;
        json_validator &operator=(json_validator const &) = delete;

        ~json_validator() = default;

        // Validate input JSON against a JSON Schema with a default throwing error reporter
        Json validate(const Json& instance) const
        {
            throwing_error_reporter reporter;
            jsonpointer::json_pointer instance_location("#");
            Json patch(json_array_arg);

            root_->validate(instance, instance_location, reporter, patch);
            return patch;
        }

        // Validate input JSON against a JSON Schema 
        bool is_valid(const Json& instance) const
        {
            fail_early_reporter reporter;
            jsonpointer::json_pointer instance_location("#");
            Json patch(json_array_arg);

            root_->validate(instance, instance_location, reporter, patch);
            return reporter.error_count() == 0;
        }

        // Validate input JSON against a JSON Schema with a provided error reporter
        template <class Reporter>
        typename std::enable_if<traits_extension::is_unary_function_object_exact<Reporter,void,validation_output>::value,Json>::type
        validate(const Json& instance, const Reporter& reporter) const
        {
            jsonpointer::json_pointer instance_location("#");
            Json patch(json_array_arg);

            error_reporter_adaptor adaptor(reporter);
            root_->validate(instance, instance_location, adaptor, patch);
            return patch;
        }
    };

} // namespace jsonschema
} // namespace turbo

#endif // JSONCONS_JSONSCHEMA_JSON_VALIDATOR_HPP
