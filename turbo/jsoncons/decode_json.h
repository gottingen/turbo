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

#ifndef JSONCONS_DECODE_JSON_HPP
#define JSONCONS_DECODE_JSON_HPP

#include <iostream>
#include <string>
#include <tuple>
#include <memory>
#include <istream> // std::basic_istream
#include "turbo/jsoncons/decode_traits.h"
#include "turbo/jsoncons/json_cursor.h"

namespace turbo {

    // decode_json

    template <class T, class Source>
    typename std::enable_if<traits_extension::is_basic_json<T>::value &&
                            traits_extension::is_sequence_of<Source,typename T::char_type>::value,T>::type
    decode_json(const Source& s,
                const basic_json_decode_options<typename Source::value_type>& options = basic_json_decode_options<typename Source::value_type>())
    {
        using char_type = typename Source::value_type;

        turbo::json_decoder<T> decoder;
        basic_json_reader<char_type, string_source<char_type>> reader(s, decoder, options);
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template <class T, class Source>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value &&
                            traits_extension::is_char_sequence<Source>::value,T>::type
    decode_json(const Source& s,
                const basic_json_decode_options<typename Source::value_type>& options = basic_json_decode_options<typename Source::value_type>())
    {
        using char_type = typename Source::value_type;

        basic_json_cursor<char_type,string_source<char_type>> cursor(s, options, default_json_parsing());
        turbo::json_decoder<basic_json<char_type>> decoder;
        std::error_code ec;
        T val = decode_traits<T,char_type>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.context().line(), cursor.context().column()));
        }
        return val;
    }

    template <class T, class CharT>
    typename std::enable_if<traits_extension::is_basic_json<T>::value,T>::type
    decode_json(std::basic_istream<CharT>& is,
                const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>())
    {
        turbo::json_decoder<T> decoder;
        basic_json_reader<CharT, stream_source<CharT>> reader(is, decoder, options);
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template <class T, class CharT>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value,T>::type
    decode_json(std::basic_istream<CharT>& is,
                const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>())
    {
        basic_json_cursor<CharT> cursor(is, options, default_json_parsing());
        json_decoder<basic_json<CharT>> decoder{};

        std::error_code ec;
        T val = decode_traits<T,CharT>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.line(), cursor.column()));
        }
        return val;
    }

    template <class T, class InputIt>
    typename std::enable_if<traits_extension::is_basic_json<T>::value,T>::type
    decode_json(InputIt first, InputIt last,
                const basic_json_decode_options<typename std::iterator_traits<InputIt>::value_type>& options = 
                    basic_json_decode_options<typename std::iterator_traits<InputIt>::value_type>())
    {
        using char_type = typename std::iterator_traits<InputIt>::value_type;

        turbo::json_decoder<T> decoder;
        basic_json_reader<char_type, iterator_source<InputIt>> reader(iterator_source<InputIt>(first,last), decoder, options);
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template <class T, class InputIt>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value,T>::type
    decode_json(InputIt first, InputIt last,
                const basic_json_decode_options<typename std::iterator_traits<InputIt>::value_type>& options = 
                    basic_json_decode_options<typename std::iterator_traits<InputIt>::value_type>())
    {
        using char_type = typename std::iterator_traits<InputIt>::value_type;

        basic_json_cursor<char_type,iterator_source<InputIt>> cursor(iterator_source<InputIt>(first, last), options, default_json_parsing());
        turbo::json_decoder<basic_json<char_type>> decoder;
        std::error_code ec;
        T val = decode_traits<T,char_type>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.line(), cursor.column()));
        }
        return val;
    }

    // With leading allocator parameter

    template <class T,class Source,class TempAllocator>
    typename std::enable_if<traits_extension::is_basic_json<T>::value &&
                            traits_extension::is_sequence_of<Source,typename T::char_type>::value,T>::type
    decode_json(temp_allocator_arg_t, const TempAllocator& temp_alloc,
                const Source& s,
                const basic_json_decode_options<typename Source::value_type>& options = basic_json_decode_options<typename Source::value_type>())
    {
        using char_type = typename Source::value_type;

        json_decoder<T,TempAllocator> decoder(temp_alloc);

        basic_json_reader<char_type, string_source<char_type>,TempAllocator> reader(s, decoder, options, temp_alloc);
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template <class T,class Source,class TempAllocator>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value &&
                            traits_extension::is_char_sequence<Source>::value,T>::type
    decode_json(temp_allocator_arg_t, const TempAllocator& temp_alloc,
                const Source& s,
                const basic_json_decode_options<typename Source::value_type>& options = basic_json_decode_options<typename Source::value_type>())
    {
        using char_type = typename Source::value_type;

        basic_json_cursor<char_type,string_source<char_type>,TempAllocator> cursor(s, options, default_json_parsing(), temp_alloc);
        json_decoder<basic_json<char_type,sorted_policy,TempAllocator>,TempAllocator> decoder(result_allocator_arg, temp_alloc, temp_alloc);

        std::error_code ec;
        T val = decode_traits<T,char_type>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.context().line(), cursor.context().column()));
        }
        return val;
    }

    template <class T,class CharT,class TempAllocator>
    typename std::enable_if<traits_extension::is_basic_json<T>::value,T>::type
    decode_json(temp_allocator_arg_t, const TempAllocator& temp_alloc,
                std::basic_istream<CharT>& is,
                const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>())
    {
        json_decoder<T,TempAllocator> decoder(temp_alloc);

        basic_json_reader<CharT, stream_source<CharT>,TempAllocator> reader(is, decoder, options, temp_alloc);
        reader.read();
        if (!decoder.is_valid())
        {
            JSONCONS_THROW(ser_error(conv_errc::conversion_failed, reader.line(), reader.column()));
        }
        return decoder.get_result();
    }

    template <class T,class CharT,class TempAllocator>
    typename std::enable_if<!traits_extension::is_basic_json<T>::value,T>::type
    decode_json(temp_allocator_arg_t, const TempAllocator& temp_alloc,
                std::basic_istream<CharT>& is,
                const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>())
    {
        basic_json_cursor<CharT,stream_source<CharT>,TempAllocator> cursor(is, options, default_json_parsing(), temp_alloc);
        json_decoder<basic_json<CharT,sorted_policy,TempAllocator>,TempAllocator> decoder(result_allocator_arg, temp_alloc,temp_alloc);

        std::error_code ec;
        T val = decode_traits<T,CharT>::decode(cursor, decoder, ec);
        if (ec)
        {
            JSONCONS_THROW(ser_error(ec, cursor.context().line(), cursor.context().column()));
        }
        return val;
    }

} // jsoncons

#endif

