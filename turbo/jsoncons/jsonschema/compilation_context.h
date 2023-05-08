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

#ifndef JSONCONS_JSONSCHEMA_COMPILATION_CONTEXT_HPP
#define JSONCONS_JSONSCHEMA_COMPILATION_CONTEXT_HPP

#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/uri.h"
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/jsonpointer/jsonpointer.h"
#include "turbo/jsoncons/jsonschema/jsonschema_error.h"
#include "turbo/jsoncons/jsonschema/schema_location.h"

namespace turbo {
namespace jsonschema {

    class compilation_context
    {
        std::vector<schema_location> uris_;
    public:
        compilation_context(const schema_location& location)
            : uris_(std::vector<schema_location>{{location}})
        {
        }

        compilation_context(schema_location&& location)
            : uris_(std::vector<schema_location>{{std::move(location)}})
        {
        }

        explicit compilation_context(const std::vector<schema_location>& uris)
            : uris_(uris)
        {
        }
        explicit compilation_context(std::vector<schema_location>&& uris)
            : uris_(std::move(uris))
        {
        }

        const std::vector<schema_location>& uris() const {return uris_;}

        std::string get_schema_path() const
        {
            return (!uris_.empty() && uris_.back().is_absolute()) ? uris_.back().string() : "";
        }

        template <class Json>
        compilation_context update_uris(const Json& schema, const std::string& key) const
        {
            return update_uris(schema, std::vector<std::string>{{key}});
        }

        template <class Json>
        compilation_context update_uris(const Json& schema, const std::vector<std::string>& keys) const
        {
            // Exclude uri's that are not plain name identifiers
            std::vector<schema_location> new_uris;
            for (const auto& uri : uris_)
            {
                if (!uri.has_identifier())
                    new_uris.push_back(uri);
            }

            // Append the keys for this sub-schema to the uri's
            for (const auto& key : keys)
            {
                for (auto& uri : new_uris)
                {
                    auto new_u = uri.append(key);
                    uri = schema_location(new_u);
                }
            }
            if (schema.type() == json_type::object_value)
            {
                auto it = schema.find("$id"); // If $id is found, this schema can be referenced by the id
                if (it != schema.object_range().end()) 
                {
                    std::string id = it->value().template as<std::string>(); 
                    // Add it to the list if it is not already there
                    if (std::find(new_uris.begin(), new_uris.end(), id) == new_uris.end())
                    {
                        schema_location relative(id); 
                        schema_location new_uri = relative.resolve(new_uris.back());
                        new_uris.emplace_back(new_uri); 
                    }
                }
            }

            return compilation_context(new_uris);
        }

        schema_location resolve_back(const schema_location& relative)
        {
            return relative.resolve(uris_.back());
        }

        std::string make_schema_path_with(const std::string& keyword) const
        {
            for (auto it = uris_.rbegin(); it != uris_.rend(); ++it)
            {
                if (!it->has_identifier() && it->is_absolute())
                {
                    return it->append(keyword).string();
                }
            }
            return "";
        }
    };

} // namespace jsonschema
} // namespace turbo

#endif // JSONCONS_JSONSCHEMA_COMPILATION_CONTEXT_HPP
