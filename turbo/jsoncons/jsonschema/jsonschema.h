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

#ifndef JSONCONS_JSONSCHEMA_JSONSCHEMA_HPP
#define JSONCONS_JSONSCHEMA_JSONSCHEMA_HPP

#include "turbo/jsoncons/jsonschema/keywords.h"
#include "turbo/jsoncons/jsonschema/json_validator.h"
#include "turbo/jsoncons/jsonschema/draft7/keyword_factory.h"


namespace turbo {
namespace jsonschema {

    template <class Json>
    using schema_draft7 = turbo::jsonschema::draft7::schema_draft7<Json>;

    template <class Json>
    std::shared_ptr<json_schema<Json>> make_schema(const Json& schema)
    {
        turbo::jsonschema::draft7::keyword_factory<Json> kwFactory{ turbo::jsonschema::draft7::default_uri_resolver<Json>()};
        kwFactory.load_root(schema);

        return kwFactory.get_schema();
    }

    template <class Json,class URIResolver>
    typename std::enable_if<traits_extension::is_unary_function_object_exact<URIResolver,Json,std::string>::value,std::shared_ptr<json_schema<Json>>>::type
    make_schema(const Json& schema, const URIResolver& resolver)
    {
        turbo::jsonschema::draft7::keyword_factory<Json> kwFactory(resolver);
        kwFactory.load_root(schema);

        return kwFactory.get_schema();
    }

} // namespace jsonschema
} // namespace turbo

#endif // JSONCONS_JSONSCHEMA_JSONSCHEMA_HPP
