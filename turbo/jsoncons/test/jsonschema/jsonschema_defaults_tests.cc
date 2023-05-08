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

#include "turbo/jsoncons/jsonschema/jsonschema.h"
#include "turbo/jsoncons/jsonpatch/jsonpatch.h"
#include "turbo/jsoncons/byte_string.h"

#include "catch/catch.hpp"
#include <fstream>
#include <iostream>
#include <regex>

using turbo::json;
namespace jsonschema = turbo::jsonschema;
namespace jsonpatch = turbo::jsonpatch;

TEST_CASE("jsonschema defaults tests")
{
    SECTION("Basic")
    {
        json schema = json::parse(R"(
{
    "properties": {
        "bar": {
            "type": "string",
            "minLength": 4,
            "default": "bad"
        }
    }
}
    )");

        try
        {
            // Data
            json data = json::parse("{\"bar\":\"bad\"}");

            // will throw schema_error if JSON Schema loading fails 
            auto sch = jsonschema::make_schema(schema); 

            jsonschema::json_validator<json> validator(sch); 

            // will throw a validation_error when a schema violation happens 
            json patch = validator.validate(data); 

            jsonpatch::apply_patch(data, patch);

            json expected = json::parse(R"(
            {"bar":"bad"}
            )");

            CHECK(data == expected);
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << "\n";
        }

    }

}

