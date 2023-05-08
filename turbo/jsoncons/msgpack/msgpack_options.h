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

#ifndef JSONCONS_MSGPACK_MSGPACK_OPTIONS_HPP
#define JSONCONS_MSGPACK_MSGPACK_OPTIONS_HPP

#include <string>
#include <limits> // std::numeric_limits
#include <cwchar>
#include "turbo/jsoncons/json_exception.h"

namespace turbo { namespace msgpack {

class msgpack_options;

class msgpack_options_common
{
    friend class msgpack_options;

    int max_nesting_depth_;
protected:
    virtual ~msgpack_options_common() = default;

    msgpack_options_common()
        : max_nesting_depth_(1024)
    {
    }

    msgpack_options_common(const msgpack_options_common&) = default;
    msgpack_options_common& operator=(const msgpack_options_common&) = default;
    msgpack_options_common(msgpack_options_common&&) = default;
    msgpack_options_common& operator=(msgpack_options_common&&) = default;
public:
    int max_nesting_depth() const 
    {
        return max_nesting_depth_;
    }
};

class msgpack_decode_options : public virtual msgpack_options_common
{
    friend class msgpack_options;
public:
    msgpack_decode_options()
    {
    }
};

class msgpack_encode_options : public virtual msgpack_options_common
{
    friend class msgpack_options;
public:
    msgpack_encode_options()
    {
    }
};

class msgpack_options final : public msgpack_decode_options, public msgpack_encode_options
{
public:
    using msgpack_options_common::max_nesting_depth;

    msgpack_options& max_nesting_depth(int value)
    {
        this->max_nesting_depth_ = value;
        return *this;
    }
};

}}
#endif
