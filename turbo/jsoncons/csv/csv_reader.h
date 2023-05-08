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

#ifndef TURBO_JSONCONS_CSV_CSV_READER_H_
#define TURBO_JSONCONS_CSV_CSV_READER_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <memory> // std::allocator
#include <utility> // std::move
#include <istream> // std::basic_istream
#include "turbo/jsoncons/source.h"
#include "turbo/jsoncons/json_exception.h"
#include "turbo/jsoncons/json_visitor.h"
#include "turbo/jsoncons/csv/csv_error.h"
#include "turbo/jsoncons/csv/csv_parser.h"
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/json_reader.h"
#include "turbo/jsoncons/json_decoder.h"
#include "turbo/jsoncons/source_adaptor.h"
#include "turbo/jsoncons/csv/csv_options.h"

namespace turbo::csv {

        template<class CharT, class Source=turbo::stream_source<CharT>, class Allocator=std::allocator<char>>
        class basic_csv_reader {
            struct stack_item {
                stack_item() noexcept
                        : array_begun_(false) {
                }

                bool array_begun_;
            };

            using char_type = CharT;
            using temp_allocator_type = Allocator;
            typedef typename std::allocator_traits<temp_allocator_type>::template rebind_alloc<CharT> char_allocator_type;

            basic_csv_reader(const basic_csv_reader &) = delete;

            basic_csv_reader &operator=(const basic_csv_reader &) = delete;

            basic_default_json_visitor<CharT> default_visitor_;
            text_source_adaptor<Source> source_;
            basic_json_visitor<CharT> &visitor_;
            basic_csv_parser<CharT, Allocator> parser_;

        public:
            // Structural characters
            static constexpr size_t default_max_buffer_size = 16384;
            //!  Parse an input stream of CSV text into a json object
            /*!
              \param is The input stream to read from
            */

            template<class Sourceable>
            basic_csv_reader(Sourceable &&source,
                             basic_json_visitor<CharT> &visitor,
                             const Allocator &alloc = Allocator())

                    : basic_csv_reader(std::forward<Sourceable>(source),
                                       visitor,
                                       basic_csv_decode_options<CharT>(),
                                       default_csv_parsing(),
                                       alloc) {
            }

            template<class Sourceable>
            basic_csv_reader(Sourceable &&source,
                             basic_json_visitor<CharT> &visitor,
                             const basic_csv_decode_options<CharT> &options,
                             const Allocator &alloc = Allocator())

                    : basic_csv_reader(std::forward<Sourceable>(source),
                                       visitor,
                                       options,
                                       default_csv_parsing(),
                                       alloc) {
            }

            template<class Sourceable>
            basic_csv_reader(Sourceable &&source,
                             basic_json_visitor<CharT> &visitor,
                             std::function<bool(csv_errc, const ser_context &)> err_handler,
                             const Allocator &alloc = Allocator())
                    : basic_csv_reader(std::forward<Sourceable>(source),
                                       visitor,
                                       basic_csv_decode_options<CharT>(),
                                       err_handler,
                                       alloc) {
            }

            template<class Sourceable>
            basic_csv_reader(Sourceable &&source,
                             basic_json_visitor<CharT> &visitor,
                             const basic_csv_decode_options<CharT> &options,
                             std::function<bool(csv_errc, const ser_context &)> err_handler,
                             const Allocator &alloc = Allocator())
                    : source_(std::forward<Sourceable>(source)),
                      visitor_(visitor),
                      parser_(options, err_handler, alloc) {
            }

            ~basic_csv_reader() noexcept = default;

            void read() {
                std::error_code ec;
                read(ec);
                if (ec) {
                    JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
                }
            }

            void read(std::error_code &ec) {
                read_internal(ec);
            }

            std::size_t line() const {
                return parser_.line();
            }

            std::size_t column() const {
                return parser_.column();
            }

            bool eof() const {
                return parser_.source_exhausted() && source_.eof();
            }

        private:

            void read_internal(std::error_code &ec) {
                if (source_.is_error()) {
                    ec = csv_errc::source_error;
                    return;
                }
                while (!parser_.stopped()) {
                    if (parser_.source_exhausted()) {
                        auto s = source_.read_buffer(ec);
                        if (ec) return;
                        if (s.size() > 0) {
                            parser_.update(s.data(), s.size());
                        }
                    }
                    parser_.parse_some(visitor_, ec);
                    if (ec) return;
                }
            }
        };

        template<class CharT, class Source=turbo::stream_source<CharT>, class Allocator=std::allocator<char>>
        class legacy_basic_csv_reader {
            struct stack_item {
                stack_item() noexcept
                        : array_begun_(false) {
                }

                bool array_begun_;
            };

            using char_type = CharT;
            using temp_allocator_type = Allocator;
            typedef typename std::allocator_traits<temp_allocator_type>::template rebind_alloc<CharT> char_allocator_type;

            legacy_basic_csv_reader(const legacy_basic_csv_reader &) = delete;

            legacy_basic_csv_reader &operator=(const legacy_basic_csv_reader &) = delete;

            basic_default_json_visitor<CharT> default_visitor_;
            text_source_adaptor<Source> source_;
            basic_json_visitor<CharT> &visitor_;
            basic_csv_parser<CharT, Allocator> parser_;

        public:
            // Structural characters
            static constexpr size_t default_max_buffer_size = 16384;
            //!  Parse an input stream of CSV text into a json object
            /*!
              \param is The input stream to read from
            */

            template<class Sourceable>
            legacy_basic_csv_reader(Sourceable &&source,
                                    basic_json_visitor<CharT> &visitor,
                                    const Allocator &alloc = Allocator())

                    : legacy_basic_csv_reader(std::forward<Sourceable>(source),
                                              visitor,
                                              basic_csv_decode_options<CharT>(),
                                              default_csv_parsing(),
                                              alloc) {
            }

            template<class Sourceable>
            legacy_basic_csv_reader(Sourceable &&source,
                                    basic_json_visitor<CharT> &visitor,
                                    const basic_csv_decode_options<CharT> &options,
                                    const Allocator &alloc = Allocator())

                    : legacy_basic_csv_reader(std::forward<Sourceable>(source),
                                              visitor,
                                              options,
                                              default_csv_parsing(),
                                              alloc) {
            }

            template<class Sourceable>
            legacy_basic_csv_reader(Sourceable &&source,
                                    basic_json_visitor<CharT> &visitor,
                                    std::function<bool(csv_errc, const ser_context &)> err_handler,
                                    const Allocator &alloc = Allocator())
                    : legacy_basic_csv_reader(std::forward<Sourceable>(source),
                                              visitor,
                                              basic_csv_decode_options<CharT>(),
                                              err_handler,
                                              alloc) {
            }

            template<class Sourceable>
            legacy_basic_csv_reader(Sourceable &&source,
                                    basic_json_visitor<CharT> &visitor,
                                    const basic_csv_decode_options<CharT> &options,
                                    std::function<bool(csv_errc, const ser_context &)> err_handler,
                                    const Allocator &alloc = Allocator(),
                                    typename std::enable_if<!std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type * = 0)
                    : source_(std::forward<Sourceable>(source)),
                      visitor_(visitor),
                      parser_(options, err_handler, alloc) {
            }

            template<class Sourceable>
            legacy_basic_csv_reader(Sourceable &&source,
                                    basic_json_visitor<CharT> &visitor,
                                    const basic_csv_decode_options<CharT> &options,
                                    std::function<bool(csv_errc, const ser_context &)> err_handler,
                                    const Allocator &alloc = Allocator(),
                                    typename std::enable_if<std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type * = 0)
                    : source_(),
                      visitor_(visitor),
                      parser_(options, err_handler, alloc) {
                turbo::basic_string_view<CharT> sv(std::forward<Sourceable>(source));
                auto r = unicode_traits::detect_encoding_from_bom(sv.data(), sv.size());
                if (!(r.encoding == unicode_traits::encoding_kind::utf8 ||
                      r.encoding == unicode_traits::encoding_kind::undetected)) {
                    JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character, parser_.line(), parser_.column()));
                }
                std::size_t offset = (r.ptr - sv.data());
                parser_.update(sv.data() + offset, sv.size() - offset);
            }

            ~legacy_basic_csv_reader() noexcept = default;

            void read() {
                std::error_code ec;
                read(ec);
                if (ec) {
                    JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
                }
            }

            void read(std::error_code &ec) {
                read_internal(ec);
            }

            std::size_t line() const {
                return parser_.line();
            }

            std::size_t column() const {
                return parser_.column();
            }

            bool eof() const {
                return parser_.source_exhausted() && source_.eof();
            }

        private:

            void read_internal(std::error_code &ec) {
                if (source_.is_error()) {
                    ec = csv_errc::source_error;
                    return;
                }
                while (!parser_.stopped()) {
                    if (parser_.source_exhausted()) {
                        auto s = source_.read_buffer(ec);
                        if (ec) return;
                        if (s.size() > 0) {
                            parser_.update(s.data(), s.size());
                        }
                    }
                    parser_.parse_some(visitor_, ec);
                    if (ec) return;
                }
            }
        };

        using csv_reader = legacy_basic_csv_reader<char>;
        using wcsv_reader = legacy_basic_csv_reader<wchar_t>;

        using csv_string_reader = basic_csv_reader<char, string_source<char>>;
        using wcsv_string_reader = basic_csv_reader<wchar_t, string_source<wchar_t>>;
        using csv_stream_reader = basic_csv_reader<char, stream_source<char>>;
        using wcsv_stream_reader = basic_csv_reader<wchar_t, stream_source<wchar_t>>;
}  // namespace turbo::csv

#endif  // TURBO_JSONCONS_CSV_CSV_READER_H_
