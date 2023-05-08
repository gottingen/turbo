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

#ifndef TURBO_JSONCONS_JSON_CURSOR_H_
#define TURBO_JSONCONS_JSON_CURSOR_H_

#include <memory> // std::allocator
#include <string>
#include <vector>
#include <stdexcept>
#include <system_error>
#include <ios>
#include <istream> // std::basic_istream
#include "turbo/jsoncons/byte_string.h"
#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/json_visitor.h"
#include "turbo/jsoncons/json_exception.h"
#include "turbo/jsoncons/json_parser.h"
#include "turbo/jsoncons/staj_cursor.h"
#include "turbo/jsoncons/source.h"
#include "turbo/jsoncons/source_adaptor.h"

namespace turbo {

    template<class CharT, class Source=turbo::stream_source<CharT>, class Allocator=std::allocator<char>>
    class basic_json_cursor : public basic_staj_cursor<CharT>, private virtual ser_context {
    public:
        using source_type = Source;
        using char_type = CharT;
        using allocator_type = Allocator;
        using string_view_type = std::basic_string_view<CharT>;
    private:
        typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<CharT> char_allocator_type;
        static constexpr size_t default_max_buffer_size = 16384;

        json_source_adaptor<Source> source_;
        basic_json_parser<CharT, Allocator> parser_;
        basic_staj_visitor<CharT> cursor_visitor_;
        bool done_;

        // Noncopyable and nonmoveable
        basic_json_cursor(const basic_json_cursor &) = delete;

        basic_json_cursor &operator=(const basic_json_cursor &) = delete;

    public:

        // Constructors that throw parse exceptions

        template<class Sourceable>
        basic_json_cursor(Sourceable &&source,
                          const basic_json_decode_options<CharT> &options = basic_json_decode_options<CharT>(),
                          std::function<bool(json_errc, const ser_context &)> err_handler = default_json_parsing(),
                          const Allocator &alloc = Allocator(),
                          typename std::enable_if<!std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type * = 0)
                : source_(std::forward<Sourceable>(source)),
                  parser_(options, err_handler, alloc),
                  cursor_visitor_(accept_all),
                  done_(false) {
            if (!done()) {
                next();
            }
        }

        template<class Sourceable>
        basic_json_cursor(Sourceable &&source,
                          const basic_json_decode_options<CharT> &options = basic_json_decode_options<CharT>(),
                          std::function<bool(json_errc, const ser_context &)> err_handler = default_json_parsing(),
                          const Allocator &alloc = Allocator(),
                          typename std::enable_if<std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type * = 0)
                : source_(),
                  parser_(options, err_handler, alloc),
                  cursor_visitor_(accept_all),
                  done_(false) {
            initialize_with_string_view(std::forward<Sourceable>(source));
        }


        // Constructors that set parse error codes
        template<class Sourceable>
        basic_json_cursor(Sourceable &&source,
                          std::error_code &ec)
                : basic_json_cursor(std::allocator_arg, Allocator(),
                                    std::forward<Sourceable>(source),
                                    basic_json_decode_options<CharT>(),
                                    default_json_parsing(),
                                    ec) {
        }

        template<class Sourceable>
        basic_json_cursor(Sourceable &&source,
                          const basic_json_decode_options<CharT> &options,
                          std::error_code &ec)
                : basic_json_cursor(std::allocator_arg, Allocator(),
                                    std::forward<Sourceable>(source),
                                    options,
                                    default_json_parsing(),
                                    ec) {
        }

        template<class Sourceable>
        basic_json_cursor(Sourceable &&source,
                          const basic_json_decode_options<CharT> &options,
                          std::function<bool(json_errc, const ser_context &)> err_handler,
                          std::error_code &ec)
                : basic_json_cursor(std::allocator_arg, Allocator(),
                                    std::forward<Sourceable>(source),
                                    options,
                                    err_handler,
                                    ec) {
        }

        template<class Sourceable>
        basic_json_cursor(std::allocator_arg_t, const Allocator &alloc,
                          Sourceable &&source,
                          const basic_json_decode_options<CharT> &options,
                          std::function<bool(json_errc, const ser_context &)> err_handler,
                          std::error_code &ec,
                          typename std::enable_if<!std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type * = 0)
                : source_(std::forward<Sourceable>(source)),
                  parser_(options, err_handler, alloc),
                  cursor_visitor_(accept_all),
                  done_(false) {
            if (!done()) {
                next(ec);
            }
        }

        template<class Sourceable>
        basic_json_cursor(std::allocator_arg_t, const Allocator &alloc,
                          Sourceable &&source,
                          const basic_json_decode_options<CharT> &options,
                          std::function<bool(json_errc, const ser_context &)> err_handler,
                          std::error_code &ec,
                          typename std::enable_if<std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type * = 0)
                : source_(),
                  parser_(options, err_handler, alloc),
                  cursor_visitor_(accept_all),
                  done_(false) {
            initialize_with_string_view(std::forward<Sourceable>(source), ec);
        }

        void reset() {
            parser_.reset();
            cursor_visitor_.reset();
            done_ = false;
            if (!done()) {
                next();
            }
        }

        template<class Sourceable>
        typename std::enable_if<!std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type
        reset(Sourceable &&source) {
            source_ = std::forward<Sourceable>(source);
            parser_.reinitialize();
            cursor_visitor_.reset();
            done_ = false;
            if (!done()) {
                next();
            }
        }

        template<class Sourceable>
        typename std::enable_if<std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type
        reset(Sourceable &&source) {
            source_ = {};
            parser_.reinitialize();
            cursor_visitor_.reset();
            done_ = false;
            initialize_with_string_view(std::forward<Sourceable>(source));
        }

        void reset(std::error_code &ec) {
            parser_.reset();
            cursor_visitor_.reset();
            done_ = false;
            if (!done()) {
                next(ec);
            }
        }

        template<class Sourceable>
        typename std::enable_if<!std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type
        reset(Sourceable &&source, std::error_code &ec) {
            source_ = std::forward<Sourceable>(source);
            parser_.reinitialize();
            cursor_visitor_.reset();
            done_ = false;
            if (!done()) {
                next(ec);
            }
        }

        template<class Sourceable>
        typename std::enable_if<std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type
        reset(Sourceable &&source, std::error_code &ec) {
            source_ = {};
            parser_.reinitialize();
            done_ = false;
            initialize_with_string_view(std::forward<Sourceable>(source), ec);
        }

        bool done() const override {
            return parser_.done() || done_;
        }

        const basic_staj_event<CharT> &current() const override {
            return cursor_visitor_.event();
        }

        void read_to(basic_json_visitor<CharT> &visitor) override {
            std::error_code ec;
            read_to(visitor, ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        void read_to(basic_json_visitor<CharT> &visitor,
                     std::error_code &ec) override {
            if (send_json_event(cursor_visitor_.event(), visitor, *this, ec)) {
                read_next(visitor, ec);
            }
        }

        void next() override {
            std::error_code ec;
            next(ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        void next(std::error_code &ec) override {
            read_next(ec);
        }

        void check_done() {
            std::error_code ec;
            check_done(ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        const ser_context &context() const override {
            return *this;
        }

        void check_done(std::error_code &ec) {
            if (source_.is_error()) {
                ec = json_errc::source_error;
                return;
            }
            if (source_.eof()) {
                parser_.check_done(ec);
                if (ec) return;
            } else {
                do {
                    if (parser_.source_exhausted()) {
                        auto s = source_.read_buffer(ec);
                        if (ec) return;
                        if (s.size() > 0) {
                            parser_.update(s.data(), s.size());
                        }
                    }
                    if (!parser_.source_exhausted()) {
                        parser_.check_done(ec);
                        if (ec) return;
                    }
                } while (!eof());
            }
        }

        bool eof() const {
            return parser_.source_exhausted() && source_.eof();
        }

        std::size_t line() const override {
            return parser_.line();
        }

        std::size_t column() const override {
            return parser_.column();
        }

        friend
        basic_staj_filter_view<CharT> operator|(basic_json_cursor &cursor,
                                                std::function<bool(const basic_staj_event<CharT> &,
                                                                   const ser_context &)> pred) {
            return basic_staj_filter_view<CharT>(cursor, pred);
        }

    private:

        static bool accept_all(const basic_staj_event<CharT> &, const ser_context &) {
            return true;
        }

        void initialize_with_string_view(string_view_type sv) {
            auto r = unicode_traits::detect_json_encoding(sv.data(), sv.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 ||
                  r.encoding == unicode_traits::encoding_kind::undetected)) {
                JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character, parser_.line(), parser_.column()));
            }
            std::size_t offset = (r.ptr - sv.data());
            parser_.update(sv.data() + offset, sv.size() - offset);

            bool is_done = parser_.done() || done_;
            if (!is_done) {
                read_next();
            }
        }

        void initialize_with_string_view(string_view_type sv, std::error_code &ec) {
            auto r = unicode_traits::detect_json_encoding(sv.data(), sv.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 ||
                  r.encoding == unicode_traits::encoding_kind::undetected)) {
                ec = json_errc::illegal_unicode_character;
                return;
            }
            std::size_t offset = (r.ptr - sv.data());
            parser_.update(sv.data() + offset, sv.size() - offset);
            bool is_done = parser_.done() || done_;
            if (!is_done) {
                read_next(ec);
            }
        }

        void read_next() {
            std::error_code ec;
            read_next(cursor_visitor_, ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        void read_next(std::error_code &ec) {
            read_next(cursor_visitor_, ec);
        }

        void read_next(basic_json_visitor<CharT> &visitor, std::error_code &ec) {
            parser_.restart();
            while (!parser_.stopped()) {
                if (parser_.source_exhausted()) {
                    auto s = source_.read_buffer(ec);
                    if (ec) return;
                    if (s.size() > 0) {
                        parser_.update(s.data(), s.size());
                        if (ec) return;
                    }
                }
                bool eof = parser_.source_exhausted() && source_.eof();
                parser_.parse_some(visitor, ec);
                if (ec) return;
                if (eof) {
                    if (parser_.enter()) {
                        done_ = true;
                        break;
                    } else if (!parser_.accept()) {
                        ec = json_errc::unexpected_eof;
                        return;
                    }
                }
            }
        }
    };

    using json_stream_cursor = basic_json_cursor<char, turbo::stream_source<char>>;
    using json_string_cursor = basic_json_cursor<char, turbo::string_source<char>>;
    using wjson_stream_cursor = basic_json_cursor<wchar_t, turbo::stream_source<wchar_t>>;
    using wjson_string_cursor = basic_json_cursor<wchar_t, turbo::string_source<wchar_t>>;

    using json_cursor = basic_json_cursor<char>;
    using wjson_cursor = basic_json_cursor<wchar_t>;


}  // namespace turbo

#endif  // TURBO_JSONCONS_JSON_CURSOR_H_


