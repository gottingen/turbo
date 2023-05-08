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

#ifndef JSONCONS_BSON_BSON_OPTIONS_HPP
#define JSONCONS_BSON_BSON_OPTIONS_HPP

#include <string>
#include <limits> // std::numeric_limits
#include <cwchar>
#include "turbo/jsoncons/json_exception.h"
#include "turbo/jsoncons/bson/bson_type.h"

namespace turbo { namespace bson {

class bson_options;

class bson_options_common
{
    friend class bson_options;

    int max_nesting_depth_;
protected:
    virtual ~bson_options_common() = default;

    bson_options_common()
        : max_nesting_depth_(1024)
    {
    }

    bson_options_common(const bson_options_common&) = default;
    bson_options_common& operator=(const bson_options_common&) = default;
    bson_options_common(bson_options_common&&) = default;
    bson_options_common& operator=(bson_options_common&&) = default;
public:
    int max_nesting_depth() const 
    {
        return max_nesting_depth_;
    }
};

class bson_decode_options : public virtual bson_options_common
{
    friend class bson_options;
public:
    bson_decode_options()
    {
    }
};

class bson_encode_options : public virtual bson_options_common
{
    friend class bson_options;
public:
    bson_encode_options()
    {
    }
};

class bson_options final : public bson_decode_options, public bson_encode_options
{
public:
    using bson_options_common::max_nesting_depth;

    bson_options& max_nesting_depth(int value)
    {
        this->max_nesting_depth_ = value;
        return *this;
    }
};

}}
#endif
