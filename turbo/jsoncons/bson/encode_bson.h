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

#ifndef JSONCONS_BSON_ENCODE_BSON_HPP
#define JSONCONS_BSON_ENCODE_BSON_HPP

#include <string>
#include <vector>
#include <memory>
#include <type_traits> // std::enable_if
#include <istream> // std::basic_istream
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/bson/bson_encoder.h"
#include "turbo/jsoncons/bson/bson_reader.h"

namespace turbo {
namespace bson {

    template<class T, class Container>
    typename std::enable_if<traits_extension::is_basic_json<T>::value &&
                            traits_extension::is_back_insertable_byte_container<Container>::value,void>::type 
    encode_bson(const T& j, 
                Container& v, 
                const bson_encode_options& options = bson_encode_options())
    {
        using char_type = typename T::char_type;
        basic_bson_encoder<turbo::bytes_sink<Container>> encoder(v, options);
        auto adaptor = make_json_visitor_adaptor<basic_json_visitor<char_type>>(encoder);
        j.dump(adaptor);
    }

    template<class T, class Container>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value &&
                            traits_extension::is_back_insertable_byte_container<Container>::value,void>::type 
    encode_bson(const T& val, 
                Container& v, 
                const bson_encode_options& options = bson_encode_options())
    {
        basic_bson_encoder<turbo::bytes_sink<Container>> encoder(v, options);
        std::error_code ec;
        encode_traits<T,char>::encode(val, encoder, json(), ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec));
        }
    }

    template<class T>
    typename std::enable_if<traits_extension::is_basic_json<T>::value,void>::type 
    encode_bson(const T& j, 
                std::ostream& os, 
                const bson_encode_options& options = bson_encode_options())
    {
        using char_type = typename T::char_type;
        bson_stream_encoder encoder(os, options);
        auto adaptor = make_json_visitor_adaptor<basic_json_visitor<char_type>>(encoder);
        j.dump(adaptor);
    }

    template<class T>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value,void>::type 
    encode_bson(const T& val, 
                std::ostream& os, 
                const bson_encode_options& options = bson_encode_options())
    {
        bson_stream_encoder encoder(os, options);
        std::error_code ec;
        encode_traits<T,char>::encode(val, encoder, json(), ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec));
        }
    }
  
    // with temp_allocator_rag

    template<class T, class Container, class TempAllocator>
    typename std::enable_if<traits_extension::is_basic_json<T>::value &&
                            traits_extension::is_back_insertable_byte_container<Container>::value,void>::type 
    encode_bson(temp_allocator_arg_t, const TempAllocator& temp_alloc,
                const T& j, 
                Container& v, 
                const bson_encode_options& options = bson_encode_options())
    {
        using char_type = typename T::char_type;
        basic_bson_encoder<turbo::bytes_sink<Container>,TempAllocator> encoder(v, options, temp_alloc);
        auto adaptor = make_json_visitor_adaptor<basic_json_visitor<char_type>>(encoder);
        j.dump(adaptor);
    }

    template<class T, class Container, class TempAllocator>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value &&
                            traits_extension::is_back_insertable_byte_container<Container>::value,void>::type 
    encode_bson(temp_allocator_arg_t, const TempAllocator& temp_alloc,
                const T& val, 
                Container& v, 
                const bson_encode_options& options = bson_encode_options())
    {
        basic_bson_encoder<turbo::bytes_sink<Container>,TempAllocator> encoder(v, options, temp_alloc);
        std::error_code ec;
        encode_traits<T,char>::encode(val, encoder, json(), ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec));
        }
    }

    template<class T,class TempAllocator>
    typename std::enable_if<traits_extension::is_basic_json<T>::value,void>::type 
    encode_bson(temp_allocator_arg_t, const TempAllocator& temp_alloc,
                const T& j, 
                std::ostream& os, 
                const bson_encode_options& options = bson_encode_options())
    {
        using char_type = typename T::char_type;
        basic_bson_encoder<turbo::binary_stream_sink,TempAllocator> encoder(os, options, temp_alloc);
        auto adaptor = make_json_visitor_adaptor<basic_json_visitor<char_type>>(encoder);
        j.dump(adaptor);
    }

    template<class T,class TempAllocator>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value,void>::type 
    encode_bson(temp_allocator_arg_t, const TempAllocator& temp_alloc,
                const T& val, 
                std::ostream& os, 
                const bson_encode_options& options = bson_encode_options())
    {
        basic_bson_encoder<turbo::binary_stream_sink,TempAllocator> encoder(os, options, temp_alloc);
        std::error_code ec;
        encode_traits<T,char>::encode(val, encoder, json(), ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec));
        }
    }
      
} // bson
} // jsoncons

#endif
