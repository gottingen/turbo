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

#ifndef JSONCONS_BUFFER_READER_HPP
#define JSONCONS_BUFFER_READER_HPP

#include <cstddef>
#include <string>
#include <vector>
#include <stdexcept>
#include <system_error>
#include <memory> // std::allocator_traits
#include <vector> // std::vector
#include "turbo/jsoncons/unicode_traits.h"
#include "turbo/jsoncons/json_error.h" // json_errc
#include "turbo/jsoncons/source.h"
#include "turbo/jsoncons/json_exception.h"

namespace turbo {

    // text_source_adaptor

    template<class Source>
    class text_source_adaptor 
    {
    public:
        using value_type = typename Source::value_type;
    private:
        Source source_;
        bool bof_;

    public:
        text_source_adaptor()
            : bof_(true)
        {
        }

        template <class Sourceable>
        text_source_adaptor(Sourceable&& source)
            : source_(std::forward<Sourceable>(source)), bof_(true)
        {
        }

        bool eof() const
        {
            return source_.eof();
        }

        bool is_error() const
        {
            return source_.is_error();  
        }

        turbo::Span<const value_type> read_buffer(std::error_code& ec)
        {
            if (source_.eof())
            {
                return turbo::Span<const value_type>();
            }

            auto s = source_.read_buffer();
            const value_type* data = s.data();
            std::size_t length = s.size();

            if (bof_ && length > 0)
            {
                auto r = unicode_traits::detect_encoding_from_bom(data, length);
                if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
                {
                    ec = json_errc::illegal_unicode_character;
                    return turbo::Span<const value_type>();
                }
                length -= (r.ptr - data);
                data = r.ptr;
                bof_ = false;
            }
            return turbo::Span<const value_type>(data, length);           
        }
    };

    // json_source_adaptor

    template<class Source>
    class json_source_adaptor 
    {
    public:
        using value_type = typename Source::value_type;
    private:
        Source source_;
        bool bof_;

    public:
        json_source_adaptor()
            : bof_(true)
        {
        }

        template <class Sourceable>
        json_source_adaptor(Sourceable&& source)
            : source_(std::forward<Sourceable>(source)), bof_(true)
        {
        }

        bool eof() const
        {
            return source_.eof();
        }

        bool is_error() const
        {
            return source_.is_error();  
        }

        turbo::Span<const value_type> read_buffer(std::error_code& ec)
        {
            if (source_.eof())
            {
                return turbo::Span<const value_type>();
            }

            auto s = source_.read_buffer();
            const value_type* data = s.data();
            std::size_t length = s.size();

            if (bof_ && length > 0)
            {
                auto r = unicode_traits::detect_json_encoding(data, length);
                if (!(r.encoding == unicode_traits::encoding_kind::utf8 || r.encoding == unicode_traits::encoding_kind::undetected))
                {
                    ec = json_errc::illegal_unicode_character;
                    return turbo::Span<const value_type>();
                }
                length -= (r.ptr - data);
                data = r.ptr;
                bof_ = false;
            }
            
            return turbo::Span<const value_type>(data, length);           
        }
    };

} // namespace turbo

#endif

