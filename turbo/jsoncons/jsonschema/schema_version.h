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

#ifndef JSONCONS_JSONSCHEMA_SCHEMA_VERSION_HPP
#define JSONCONS_JSONSCHEMA_SCHEMA_VERSION_HPP

#include "turbo/jsoncons/json.h"

namespace turbo {
namespace jsonschema {

    class schema_version
    {
    public:
        static bool contains(const string_view& url)
        {
            if (url.find("json-schema.org/draft-07/schema#") != string_view::npos)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    };


} // namespace jsonschema
} // namespace turbo

#endif // JSONCONS_JSONSCHEMA_SCHEMA_VERSION_HPP
