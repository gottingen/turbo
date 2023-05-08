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

#ifndef JSONCONS_PRETTY_PRINT_HPP
#define JSONCONS_PRETTY_PRINT_HPP

#include <string>
#include <exception>
#include <cstring>
#include <ostream>
#include <memory>
#include <typeinfo>
#include <cstring>
#include "turbo/jsoncons/json_exception.h"
#include "turbo/jsoncons/json_options.h"
#include "turbo/jsoncons/json_encoder.h"
#include "turbo/jsoncons/json_type_traits.h"
#include "turbo/jsoncons/json_error.h"

namespace turbo {

template<class Json>
class json_printable
{
public:
    using char_type = typename Json::char_type;

    json_printable(const Json& j, indenting line_indent)
       : j_(&j), indenting_(line_indent)
    {
    }

    json_printable(const Json& j,
                   const basic_json_encode_options<char_type>& options,
                   indenting line_indent)
       : j_(&j), options_(options), indenting_(line_indent)
    {
    }

    void dump(std::basic_ostream<char_type>& os) const
    {
        j_->dump(os, options_, indenting_);
    }

    friend std::basic_ostream<char_type>& operator<<(std::basic_ostream<char_type>& os, const json_printable<Json>& pr)
    {
        pr.dump(os);
        return os;
    }

    const Json *j_;
    basic_json_encode_options<char_type> options_;
    indenting indenting_;
private:
    json_printable();
};

template<class Json>
json_printable<Json> print(const Json& j)
{
    return json_printable<Json>(j, indenting::no_indent);
}

template<class Json>
json_printable<Json> print(const Json& j,
                           const basic_json_encode_options<typename Json::char_type>& options)
{
    return json_printable<Json>(j, options, indenting::no_indent);
}

template<class Json>
json_printable<Json> pretty_print(const Json& j)
{
    return json_printable<Json>(j, indenting::indent);
}

template<class Json>
json_printable<Json> pretty_print(const Json& j,
                                  const basic_json_encode_options<typename Json::char_type>& options)
{
    return json_printable<Json>(j, options, indenting::indent);
}

}

#endif
