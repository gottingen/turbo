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

#ifndef JSONCONS_JSON_READER_HPP
#define JSONCONS_JSON_READER_HPP

#include <memory> // std::allocator
#include <string>
#include <vector>
#include <stdexcept>
#include <system_error>
#include <ios>
#include <utility> // std::move
#include "turbo/jsoncons/source.h"
#include "turbo/jsoncons/json_exception.h"
#include "turbo/jsoncons/json_visitor.h"
#include "turbo/jsoncons/json_parser.h"
#include "turbo/jsoncons/source_adaptor.h"

namespace turbo {

    // utf8_other_json_input_adapter

    template<class CharT>
    class json_utf8_to_other_visitor_adaptor : public json_visitor {
    public:
        using json_visitor::string_view_type;
    private:
        basic_default_json_visitor<CharT> default_visitor_;
        basic_json_visitor<CharT> &other_visitor_;
        //std::function<bool(json_errc,const ser_context&)> err_handler_;

        // noncopyable and nonmoveable
        json_utf8_to_other_visitor_adaptor(const json_utf8_to_other_visitor_adaptor<CharT> &) = delete;

        json_utf8_to_other_visitor_adaptor<CharT> &
        operator=(const json_utf8_to_other_visitor_adaptor<CharT> &) = delete;

    public:
        json_utf8_to_other_visitor_adaptor()
                : other_visitor_(default_visitor_) {
        }

        json_utf8_to_other_visitor_adaptor(basic_json_visitor<CharT> &other_visitor/*,
                                              std::function<bool(json_errc,const ser_context&)> err_handler*/)
                : other_visitor_(other_visitor)/*,
              err_handler_(err_handler)*/
        {
        }

    private:

        void visit_flush() override {
            other_visitor_.flush();
        }

        bool visit_begin_object(semantic_tag tag, const ser_context &context, std::error_code &ec) override {
            return other_visitor_.begin_object(tag, context, ec);
        }

        bool visit_end_object(const ser_context &context, std::error_code &ec) override {
            return other_visitor_.end_object(context, ec);
        }

        bool visit_begin_array(semantic_tag tag, const ser_context &context, std::error_code &ec) override {
            return other_visitor_.begin_array(tag, context, ec);
        }

        bool visit_end_array(const ser_context &context, std::error_code &ec) override {
            return other_visitor_.end_array(context, ec);
        }

        bool visit_key(const string_view_type &name, const ser_context &context, std::error_code &ec) override {
            std::basic_string<CharT> target;
            auto result = unicode_traits::convert(
                    name.data(), name.size(), target,
                    unicode_traits::conv_flags::strict);
            if (result.ec != unicode_traits::conv_errc()) {
                JSONCONS_THROW(ser_error(result.ec, context.line(), context.column()));
            }
            return other_visitor_.key(target, context, ec);
        }

        bool visit_string(const string_view_type &value, semantic_tag tag, const ser_context &context,
                          std::error_code &ec) override {
            std::basic_string<CharT> target;
            auto result = unicode_traits::convert(
                    value.data(), value.size(), target,
                    unicode_traits::conv_flags::strict);
            if (result.ec != unicode_traits::conv_errc()) {
                ec = result.ec;
                return false;
            }
            return other_visitor_.string_value(target, tag, context, ec);
        }

        bool visit_int64(int64_t value,
                         semantic_tag tag,
                         const ser_context &context,
                         std::error_code &ec) override {
            return other_visitor_.int64_value(value, tag, context, ec);
        }

        bool visit_uint64(uint64_t value,
                          semantic_tag tag,
                          const ser_context &context,
                          std::error_code &ec) override {
            return other_visitor_.uint64_value(value, tag, context, ec);
        }

        bool visit_half(uint16_t value,
                        semantic_tag tag,
                        const ser_context &context,
                        std::error_code &ec) override {
            return other_visitor_.half_value(value, tag, context, ec);
        }

        bool visit_double(double value,
                          semantic_tag tag,
                          const ser_context &context,
                          std::error_code &ec) override {
            return other_visitor_.double_value(value, tag, context, ec);
        }

        bool visit_bool(bool value, semantic_tag tag, const ser_context &context, std::error_code &ec) override {
            return other_visitor_.bool_value(value, tag, context, ec);
        }

        bool visit_null(semantic_tag tag, const ser_context &context, std::error_code &ec) override {
            return other_visitor_.null_value(tag, context, ec);
        }
    };

    template<class CharT, class Source=turbo::stream_source<CharT>, class Allocator=std::allocator<char>>
    class basic_json_reader {
    public:
        using char_type = CharT;
        using source_type = Source;
        using string_view_type = turbo::basic_string_view<CharT>;
    private:
        typedef typename std::allocator_traits<Allocator>::template rebind_alloc<CharT> char_allocator_type;

        static constexpr size_t default_max_buffer_size = 16384;

        json_source_adaptor<Source> source_;
        basic_default_json_visitor<CharT> default_visitor_;
        basic_json_visitor<CharT> &visitor_;
        basic_json_parser<CharT, Allocator> parser_;

        // Noncopyable and nonmoveable
        basic_json_reader(const basic_json_reader &) = delete;

        basic_json_reader &operator=(const basic_json_reader &) = delete;

    public:
        template<class Sourceable>
        explicit basic_json_reader(Sourceable &&source, const Allocator &alloc = Allocator())
                : basic_json_reader(std::forward<Sourceable>(source),
                                    default_visitor_,
                                    basic_json_decode_options<CharT>(),
                                    default_json_parsing(),
                                    alloc) {
        }

        template<class Sourceable>
        basic_json_reader(Sourceable &&source,
                          const basic_json_decode_options<CharT> &options,
                          const Allocator &alloc = Allocator())
                : basic_json_reader(std::forward<Sourceable>(source),
                                    default_visitor_,
                                    options,
                                    default_json_parsing(),
                                    alloc) {
        }

        template<class Sourceable>
        basic_json_reader(Sourceable &&source,
                          std::function<bool(json_errc, const ser_context &)> err_handler,
                          const Allocator &alloc = Allocator())
                : basic_json_reader(std::forward<Sourceable>(source),
                                    default_visitor_,
                                    basic_json_decode_options<CharT>(),
                                    err_handler,
                                    alloc) {
        }

        template<class Sourceable>
        basic_json_reader(Sourceable &&source,
                          const basic_json_decode_options<CharT> &options,
                          std::function<bool(json_errc, const ser_context &)> err_handler,
                          const Allocator &alloc = Allocator())
                : basic_json_reader(std::forward<Sourceable>(source),
                                    default_visitor_,
                                    options,
                                    err_handler,
                                    alloc) {
        }

        template<class Sourceable>
        basic_json_reader(Sourceable &&source,
                          basic_json_visitor<CharT> &visitor,
                          const Allocator &alloc = Allocator())
                : basic_json_reader(std::forward<Sourceable>(source),
                                    visitor,
                                    basic_json_decode_options<CharT>(),
                                    default_json_parsing(),
                                    alloc) {
        }

        template<class Sourceable>
        basic_json_reader(Sourceable &&source,
                          basic_json_visitor<CharT> &visitor,
                          const basic_json_decode_options<CharT> &options,
                          const Allocator &alloc = Allocator())
                : basic_json_reader(std::forward<Sourceable>(source),
                                    visitor,
                                    options,
                                    default_json_parsing(),
                                    alloc) {
        }

        template<class Sourceable>
        basic_json_reader(Sourceable &&source,
                          basic_json_visitor<CharT> &visitor,
                          std::function<bool(json_errc, const ser_context &)> err_handler,
                          const Allocator &alloc = Allocator())
                : basic_json_reader(std::forward<Sourceable>(source),
                                    visitor,
                                    basic_json_decode_options<CharT>(),
                                    err_handler,
                                    alloc) {
        }

        template<class Sourceable>
        basic_json_reader(Sourceable &&source,
                          basic_json_visitor<CharT> &visitor,
                          const basic_json_decode_options<CharT> &options,
                          std::function<bool(json_errc, const ser_context &)> err_handler,
                          const Allocator &alloc = Allocator())
                : source_(std::forward<Sourceable>(source)),
                  visitor_(visitor),
                  parser_(options, err_handler, alloc) {
        }

        void read_next() {
            std::error_code ec;
            read_next(ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        void read_next(std::error_code &ec) {
            if (source_.is_error()) {
                ec = json_errc::source_error;
                return;
            }
            parser_.reset();
            while (!parser_.stopped()) {
                if (parser_.source_exhausted()) {
                    auto s = source_.read_buffer(ec);
                    if (ec) return;
                    if (s.size() > 0) {
                        parser_.update(s.data(), s.size());
                    }
                }
                bool eof = parser_.source_exhausted();
                parser_.parse_some(visitor_, ec);
                if (ec) return;
                if (eof) {
                    if (parser_.enter()) {
                        break;
                    } else if (!parser_.accept()) {
                        ec = json_errc::unexpected_eof;
                        return;
                    }
                }
            }

            while (!source_.eof()) {
                parser_.skip_whitespace();
                if (parser_.source_exhausted()) {
                    auto s = source_.read_buffer(ec);
                    if (ec) return;
                    if (s.size() > 0) {
                        parser_.update(s.data(), s.size());
                    }
                } else {
                    break;
                }
            }
        }

        void check_done() {
            std::error_code ec;
            check_done(ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        std::size_t line() const {
            return parser_.line();
        }

        std::size_t column() const {
            return parser_.column();
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

        void read() {
            read_next();
            check_done();
        }

        void read(std::error_code &ec) {
            read_next(ec);
            if (!ec) {
                check_done(ec);
            }
        }
    };

    template<class CharT, class Source=turbo::stream_source<CharT>, class Allocator=std::allocator<char>>
    class legacy_basic_json_reader {
    public:
        using char_type = CharT;
        using source_type = Source;
        using string_view_type = turbo::basic_string_view<CharT>;
    private:
        typedef typename std::allocator_traits<Allocator>::template rebind_alloc<CharT> char_allocator_type;

        static constexpr size_t default_max_buffer_size = 16384;

        json_source_adaptor<Source> source_;
        basic_default_json_visitor<CharT> default_visitor_;
        basic_json_visitor<CharT> &visitor_;
        basic_json_parser<CharT, Allocator> parser_;

        // Noncopyable and nonmoveable
        legacy_basic_json_reader(const legacy_basic_json_reader &) = delete;

        legacy_basic_json_reader &operator=(const legacy_basic_json_reader &) = delete;

    public:
        template<class Sourceable>
        explicit legacy_basic_json_reader(Sourceable &&source, const Allocator &alloc = Allocator())
                : legacy_basic_json_reader(std::forward<Sourceable>(source),
                                           default_visitor_,
                                           basic_json_decode_options<CharT>(),
                                           default_json_parsing(),
                                           alloc) {
        }

        template<class Sourceable>
        legacy_basic_json_reader(Sourceable &&source,
                                 const basic_json_decode_options<CharT> &options,
                                 const Allocator &alloc = Allocator())
                : legacy_basic_json_reader(std::forward<Sourceable>(source),
                                           default_visitor_,
                                           options,
                                           default_json_parsing(),
                                           alloc) {
        }

        template<class Sourceable>
        legacy_basic_json_reader(Sourceable &&source,
                                 std::function<bool(json_errc, const ser_context &)> err_handler,
                                 const Allocator &alloc = Allocator())
                : legacy_basic_json_reader(std::forward<Sourceable>(source),
                                           default_visitor_,
                                           basic_json_decode_options<CharT>(),
                                           err_handler,
                                           alloc) {
        }

        template<class Sourceable>
        legacy_basic_json_reader(Sourceable &&source,
                                 const basic_json_decode_options<CharT> &options,
                                 std::function<bool(json_errc, const ser_context &)> err_handler,
                                 const Allocator &alloc = Allocator())
                : legacy_basic_json_reader(std::forward<Sourceable>(source),
                                           default_visitor_,
                                           options,
                                           err_handler,
                                           alloc) {
        }

        template<class Sourceable>
        legacy_basic_json_reader(Sourceable &&source,
                                 basic_json_visitor<CharT> &visitor,
                                 const Allocator &alloc = Allocator())
                : legacy_basic_json_reader(std::forward<Sourceable>(source),
                                           visitor,
                                           basic_json_decode_options<CharT>(),
                                           default_json_parsing(),
                                           alloc) {
        }

        template<class Sourceable>
        legacy_basic_json_reader(Sourceable &&source,
                                 basic_json_visitor<CharT> &visitor,
                                 const basic_json_decode_options<CharT> &options,
                                 const Allocator &alloc = Allocator())
                : legacy_basic_json_reader(std::forward<Sourceable>(source),
                                           visitor,
                                           options,
                                           default_json_parsing(),
                                           alloc) {
        }

        template<class Sourceable>
        legacy_basic_json_reader(Sourceable &&source,
                                 basic_json_visitor<CharT> &visitor,
                                 std::function<bool(json_errc, const ser_context &)> err_handler,
                                 const Allocator &alloc = Allocator())
                : legacy_basic_json_reader(std::forward<Sourceable>(source),
                                           visitor,
                                           basic_json_decode_options<CharT>(),
                                           err_handler,
                                           alloc) {
        }

        template<class Sourceable>
        legacy_basic_json_reader(Sourceable &&source,
                                 basic_json_visitor<CharT> &visitor,
                                 const basic_json_decode_options<CharT> &options,
                                 std::function<bool(json_errc, const ser_context &)> err_handler,
                                 const Allocator &alloc = Allocator(),
                                 typename std::enable_if<!std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type * = 0)
                : source_(std::forward<Sourceable>(source)),
                  visitor_(visitor),
                  parser_(options, err_handler, alloc) {
        }

        template<class Sourceable>
        legacy_basic_json_reader(Sourceable &&source,
                                 basic_json_visitor<CharT> &visitor,
                                 const basic_json_decode_options<CharT> &options,
                                 std::function<bool(json_errc, const ser_context &)> err_handler,
                                 const Allocator &alloc = Allocator(),
                                 typename std::enable_if<std::is_constructible<turbo::basic_string_view<CharT>, Sourceable>::value>::type * = 0)
                : source_(),
                  visitor_(visitor),
                  parser_(options, err_handler, alloc) {
            turbo::basic_string_view<CharT> sv(std::forward<Sourceable>(source));

            auto r = unicode_traits::detect_json_encoding(sv.data(), sv.size());
            if (!(r.encoding == unicode_traits::encoding_kind::utf8 ||
                  r.encoding == unicode_traits::encoding_kind::undetected)) {
                JSONCONS_THROW(ser_error(json_errc::illegal_unicode_character, parser_.line(), parser_.column()));
            }
            std::size_t offset = (r.ptr - sv.data());
            parser_.update(sv.data() + offset, sv.size() - offset);
        }

        void read_next() {
            std::error_code ec;
            read_next(ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        void read_next(std::error_code &ec) {
            if (source_.is_error()) {
                ec = json_errc::source_error;
                return;
            }
            parser_.reset();
            while (!parser_.stopped()) {
                if (parser_.source_exhausted()) {
                    auto s = source_.read_buffer(ec);
                    if (ec) return;
                    if (s.size() > 0) {
                        parser_.update(s.data(), s.size());
                    }
                }
                bool eof = parser_.source_exhausted();
                parser_.parse_some(visitor_, ec);
                if (ec) return;
                if (eof) {
                    if (parser_.enter()) {
                        break;
                    } else if (!parser_.accept()) {
                        ec = json_errc::unexpected_eof;
                        return;
                    }
                }
            }

            while (!source_.eof()) {
                parser_.skip_whitespace();
                if (parser_.source_exhausted()) {
                    auto s = source_.read_buffer(ec);
                    if (ec) return;
                    if (s.size() > 0) {
                        parser_.update(s.data(), s.size());
                    }
                } else {
                    break;
                }
            }
        }

        void check_done() {
            std::error_code ec;
            check_done(ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        std::size_t line() const {
            return parser_.line();
        }

        std::size_t column() const {
            return parser_.column();
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

        void read() {
            read_next();
            check_done();
        }

        void read(std::error_code &ec) {
            read_next(ec);
            if (!ec) {
                check_done(ec);
            }
        }
    };

    using json_string_reader = basic_json_reader<char, string_source<char>>;
    using wjson_string_reader = basic_json_reader<wchar_t, string_source<wchar_t>>;
    using json_stream_reader = basic_json_reader<char, stream_source<char>>;
    using wjson_stream_reader = basic_json_reader<wchar_t, stream_source<wchar_t>>;
}

#endif

