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

#ifndef JSONCONS_UBJSON_UBJSON_READER_HPP
#define JSONCONS_UBJSON_UBJSON_READER_HPP

#include <string>
#include <memory>
#include <utility> // std::move
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/source.h"
#include "turbo/jsoncons/json_visitor.h"
#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/ubjson/ubjson_type.h"
#include "turbo/jsoncons/ubjson/ubjson_error.h"
#include "turbo/jsoncons/ubjson/ubjson_parser.h"

namespace turbo { namespace ubjson {

template <class Source,class Allocator=std::allocator<char>>
class basic_ubjson_reader
{
    basic_ubjson_parser<Source,Allocator> parser_;
    json_visitor& visitor_;
public:
    template <class Sourceable>
    basic_ubjson_reader(Sourceable&& source, 
                      json_visitor& visitor, 
                      const Allocator alloc)
       : basic_ubjson_reader(std::forward<Sourceable>(source),
                           visitor,
                           ubjson_decode_options(),
                           alloc)
    {
    }

    template <class Sourceable>
    basic_ubjson_reader(Sourceable&& source, 
                      json_visitor& visitor, 
                      const ubjson_decode_options& options = ubjson_decode_options(),
                      const Allocator alloc=Allocator())
       : parser_(std::forward<Sourceable>(source), options, alloc),
         visitor_(visitor)
    {
    }

    void read()
    {
        std::error_code ec;
        read(ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec,line(),column()));
        }
    }

    void read(std::error_code& ec)
    {
        parser_.reset();
        parser_.parse(visitor_, ec);
        if (ec)
        {
            return;
        }
    }

    std::size_t line() const 
    {
        return parser_.line();
    }

    std::size_t column() const
    {
        return parser_.column();
    }
};

using ubjson_stream_reader = basic_ubjson_reader<turbo::binary_stream_source>;

using ubjson_bytes_reader = basic_ubjson_reader<turbo::bytes_source>;

#if !defined(JSONCONS_NO_DEPRECATED)
JSONCONS_DEPRECATED_MSG("Instead, use ubjson_stream_reader") typedef ubjson_stream_reader ubjson_reader;
JSONCONS_DEPRECATED_MSG("Instead, use ubjson_bytes_reader") typedef ubjson_bytes_reader ubjson_buffer_reader;
#endif

}}

#endif
