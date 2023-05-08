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

#ifndef JSONCONS_JSONSCHEMA_SCHEMA_HPP
#define JSONCONS_JSONSCHEMA_SCHEMA_HPP

#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/uri.h"
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/jsonpointer/jsonpointer.h"
#include "turbo/jsoncons/jsonschema/jsonschema_error.h"
#include "turbo/jsoncons/jsonschema/schema_location.h"
#include "turbo/jsoncons/jsonschema/keyword_validator.h"

namespace turbo {
namespace jsonschema {

    template <class Json>
    class json_schema
    {
        using validator_type = typename std::unique_ptr<keyword_validator<Json>>;

        std::vector<validator_type> subschemas_;
        validator_type root_;
    public:
        json_schema(std::vector<validator_type>&& subschemas, validator_type&& root)
            : subschemas_(std::move(subschemas)), root_(std::move(root))
        {
            if (root_ == nullptr)
                JSONCONS_THROW(schema_error("There is no root schema to validate an instance against"));
        }

        json_schema(const json_schema&) = delete;
        json_schema(json_schema&&) = default;
        json_schema& operator=(const json_schema&) = delete;
        json_schema& operator=(json_schema&&) = default;

        void validate(const Json& instance, 
                      const jsonpointer::json_pointer& instance_location, 
                      error_reporter& reporter, 
                      Json& patch) const 
        {
            TURBO_ASSERT(root_ != nullptr);
            root_->validate(instance, instance_location, reporter, patch);
        }
    };


} // namespace jsonschema
} // namespace turbo

#endif // JSONCONS_JSONSCHEMA_SCHEMA_HPP
