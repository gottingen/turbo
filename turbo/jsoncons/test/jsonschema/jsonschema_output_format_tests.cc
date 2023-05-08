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
#include "turbo/jsoncons/byte_string.h"

#include "catch/catch.hpp"
#include <fstream>
#include <iostream>
#include <regex>

using turbo::json;
namespace jsonschema = turbo::jsonschema;

TEST_CASE("jsonschema output format tests")
{
    SECTION("Basic")
    {
        json schema = json::parse(R"(
{
  "$id": "https://example.com/polygon",
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$defs": {
    "point": {
      "type": "object",
      "properties": {
        "x": { "type": "number" },
        "y": { "type": "number" }
      },
      "additionalProperties": false,
      "required": [ "x", "y" ]
    }
  },
  "type": "array",
  "items": { "$ref": "#/$defs/point" },
  "minItems": 3,
  "maxItems": 1
}

        )");

        json instance = json::parse(R"(
[
  {
    "x": 2.5,
    "y": 1.3
  },
  {
    "x": 1,
    "z": 6.7
  }
]
        )");

        auto sch = jsonschema::make_schema(schema);
        jsonschema::json_validator<json> validator(sch);

        auto reporter = [](const jsonschema::validation_output& o)
        {
            if (o.keyword() == "minItems")
            {
                CHECK(o.schema_path() == std::string("https://example.com/polygon#/minItems"));
            }
            else if (o.keyword() == "maxItems")
            {
                CHECK(o.schema_path() == std::string("https://example.com/polygon#/maxItems"));
            }
            else if (o.keyword() == "required")
            {
                CHECK(o.schema_path() == std::string("https://example.com/polygon#/$defs/point/required"));
            }
            else if (o.keyword() == "additionalProperties")
            {
                CHECK(o.schema_path() == std::string("https://example.com/polygon#/$defs/point/additionalProperties/false"));
            }
            else
            {
                std::cout << o.keyword() << ", " << o.instance_location() << ": " << o.message() << ", " << o.schema_path() << "\n";
                for (const auto& nested : o.nested_errors())
                {
                    std::cout << "    " << nested.message() << "\n";
                }
            }
        };
        validator.validate(instance, reporter);

    }
}

/*
: Expected minimum item count: 3, found: 2
/1: Required key "y" not found
/1: Validation failed for additional property "z". False schema always fails
*/
