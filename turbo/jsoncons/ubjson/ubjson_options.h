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

#ifndef JSONCONS_UBJSON_UBJSON_OPTIONS_HPP
#define JSONCONS_UBJSON_UBJSON_OPTIONS_HPP

#include <string>
#include <limits> // std::numeric_limits
#include <cwchar>
#include "turbo/jsoncons/json_exception.h"

namespace turbo { namespace ubjson {

class ubjson_options;

class ubjson_options_common
{
    friend class ubjson_options;

    int max_nesting_depth_;
protected:
    virtual ~ubjson_options_common() = default;

    ubjson_options_common()
        : max_nesting_depth_(1024)
    {
    }

    ubjson_options_common(const ubjson_options_common&) = default;
    ubjson_options_common& operator=(const ubjson_options_common&) = default;
    ubjson_options_common(ubjson_options_common&&) = default;
    ubjson_options_common& operator=(ubjson_options_common&&) = default;
public:
    int max_nesting_depth() const 
    {
        return max_nesting_depth_;
    }
};

class ubjson_decode_options : public virtual ubjson_options_common
{
    friend class ubjson_options;
    std::size_t max_items_;
public:
    ubjson_decode_options() :
         max_items_(1 << 24)
    {
    }

    std::size_t max_items() const
    {
        return max_items_;
    }
};

class ubjson_encode_options : public virtual ubjson_options_common
{
    friend class ubjson_options;
public:
    ubjson_encode_options()
    {
    }
};

class ubjson_options final : public ubjson_decode_options, public ubjson_encode_options
{
public:
    using ubjson_options_common::max_nesting_depth;

    ubjson_options& max_nesting_depth(int value)
    {
        this->max_nesting_depth_ = value;
        return *this;
    }

    ubjson_options& max_items(std::size_t value)
    {
        this->max_items_ = value;
        return *this;
    }
};

}}
#endif
