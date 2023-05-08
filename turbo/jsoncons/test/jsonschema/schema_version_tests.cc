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

TEST_CASE("jsonschema version tests")
{
    json schema_04 = json::parse(R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "description": "A product from Acme's catalog",
    "properties": {
      "id": {
        "description": "The unique identifier for a product",
        "type": "integer"
      },
      "name": {
        "description": "Name of the product",
        "type": "string"
      },
      "price": {
        "exclusiveMinimum": true,
        "minimum": 0,
        "type": "number"
      },
      "tags": {
        "items": {
          "type": "string"
        },
        "minItems": 1,
        "type": "array",
        "uniqueItems": true
      }
    },
    "required": ["id", "name", "price"],
    "title": "Product",
    "type": "object"
  }
      )");

    json schema_07 = json::parse(R"(
{
    "$schema": "http://json-schema.org/draft-07/schema#",
    "description": "A product from Acme's catalog",
    "properties": {
      "id": {
        "description": "The unique identifier for a product",
        "type": "integer"
      },
      "name": {
        "description": "Name of the product",
        "type": "string"
      },
      "price": {
        "exclusiveMinimum": true,
        "minimum": 0,
        "type": "number"
      },
      "tags": {
        "items": {
          "type": "string"
        },
        "minItems": 1,
        "type": "array",
        "uniqueItems": true
      }
    },
    "required": ["id", "name", "price"],
    "title": "Product",
    "type": "object"
  }
      )");

    SECTION("test 4")
    {
        REQUIRE_THROWS_WITH(jsonschema::make_schema(schema_04), "Unsupported schema version http://json-schema.org/draft-04/schema#");
    }

    SECTION("test 7")
    {
        REQUIRE_THROWS_WITH(jsonschema::make_schema(schema_07), "exclusiveMinimum must be a number value");
    }
}

