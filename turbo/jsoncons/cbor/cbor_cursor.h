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

#ifndef TURBO_JSONCONS_CBOR_CBOR_CURSOR_H_
#define TURBO_JSONCONS_CBOR_CBOR_CURSOR_H_

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
#include "turbo/jsoncons/staj_cursor.h"
#include "turbo/jsoncons/source.h"
#include "turbo/jsoncons/cbor/cbor_parser.h"

namespace turbo::cbor {


    template<class Source=turbo::binary_stream_source, class Allocator=std::allocator<char>>
    class basic_cbor_cursor : public basic_staj_cursor<char>, private virtual ser_context {
    public:
        using source_type = Source;
        using char_type = char;
        using allocator_type = Allocator;
    private:
        basic_cbor_parser<Source, Allocator> parser_;
        basic_staj_visitor<char_type> cursor_visitor_;
        basic_item_event_visitor_to_json_visitor<char_type, Allocator> cursor_handler_adaptor_;
        bool eof_;

        // Noncopyable and nonmoveable
        basic_cbor_cursor(const basic_cbor_cursor &) = delete;

        basic_cbor_cursor &operator=(const basic_cbor_cursor &) = delete;

    public:
        using string_view_type = string_view;

        template<class Sourceable>
        basic_cbor_cursor(Sourceable &&source,
                          const cbor_decode_options &options = cbor_decode_options(),
                          const Allocator &alloc = Allocator())
                : parser_(std::forward<Sourceable>(source), options, alloc),
                  cursor_visitor_(accept_all),
                  cursor_handler_adaptor_(cursor_visitor_, alloc),
                  eof_(false) {
            if (!done()) {
                next();
            }
        }

        // Constructors that set parse error codes

        template<class Sourceable>
        basic_cbor_cursor(Sourceable &&source,
                          std::error_code &ec)
                : basic_cbor_cursor(std::allocator_arg, Allocator(),
                                    std::forward<Sourceable>(source),
                                    cbor_decode_options(),
                                    ec) {
        }

        template<class Sourceable>
        basic_cbor_cursor(Sourceable &&source,
                          const cbor_decode_options &options,
                          std::error_code &ec)
                : basic_cbor_cursor(std::allocator_arg, Allocator(),
                                    std::forward<Sourceable>(source),
                                    options,
                                    ec) {
        }

        template<class Sourceable>
        basic_cbor_cursor(std::allocator_arg_t, const Allocator &alloc,
                          Sourceable &&source,
                          const cbor_decode_options &options,
                          std::error_code &ec)
                : parser_(std::forward<Sourceable>(source), options, alloc),
                  cursor_visitor_(accept_all),
                  cursor_handler_adaptor_(cursor_visitor_, alloc),
                  eof_(false) {
            if (!done()) {
                next(ec);
            }
        }

        void reset() {
            parser_.reset();
            cursor_visitor_.reset();
            cursor_handler_adaptor_.reset();
            eof_ = false;
            if (!done()) {
                next();
            }
        }

        template<class Sourceable>
        void reset(Sourceable &&source) {
            parser_.reset(std::forward<Sourceable>(source));
            cursor_visitor_.reset();
            cursor_handler_adaptor_.reset();
            eof_ = false;
            if (!done()) {
                next();
            }
        }

        void reset(std::error_code &ec) {
            parser_.reset();
            cursor_visitor_.reset();
            cursor_handler_adaptor_.reset();
            eof_ = false;
            if (!done()) {
                next(ec);
            }
        }

        template<class Sourceable>
        void reset(Sourceable &&source, std::error_code &ec) {
            parser_.reset(std::forward<Sourceable>(source));
            cursor_visitor_.reset();
            cursor_handler_adaptor_.reset();
            eof_ = false;
            if (!done()) {
                next(ec);
            }
        }

        bool done() const override {
            return parser_.done();
        }

        bool is_typed_array() const {
            return cursor_visitor_.is_typed_array();
        }

        const staj_event &current() const override {
            return cursor_visitor_.event();
        }

        void read_to(basic_json_visitor<char_type> &visitor) override {
            std::error_code ec;
            read_to(visitor, ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, parser_.line(), parser_.column()));
            }
        }

        void read_to(basic_json_visitor<char_type> &visitor,
                     std::error_code &ec) override {
            if (cursor_visitor_.dump(visitor, *this, ec)) {
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

        const ser_context &context() const override {
            return *this;
        }

        bool eof() const {
            return eof_;
        }

        std::size_t line() const override {
            return parser_.line();
        }

        std::size_t column() const override {
            return parser_.column();
        }

        friend
        staj_filter_view operator|(basic_cbor_cursor &cursor,
                                   std::function<bool(const staj_event &, const ser_context &)> pred) {
            return staj_filter_view(cursor, pred);
        }

    private:
        static bool accept_all(const staj_event &, const ser_context &) {
            return true;
        }

        void read_next(std::error_code &ec) {
            if (cursor_visitor_.in_available()) {
                cursor_visitor_.send_available(ec);
            } else {
                parser_.restart();
                while (!parser_.stopped()) {
                    parser_.parse(cursor_handler_adaptor_, ec);
                    if (ec) return;
                }
            }
        }

        void read_next(basic_json_visitor<char_type> &visitor, std::error_code &ec) {
            {
                struct resource_wrapper {
                    basic_item_event_visitor_to_json_visitor<char_type, Allocator> &adaptor;
                    basic_json_visitor<char_type> &original;

                    resource_wrapper(basic_item_event_visitor_to_json_visitor<char_type, Allocator> &adaptor,
                                     basic_json_visitor<char_type> &visitor)
                            : adaptor(adaptor), original(adaptor.destination()) {
                        adaptor.destination(visitor);
                    }

                    ~resource_wrapper() {
                        adaptor.destination(original);
                    }
                } wrapper(cursor_handler_adaptor_, visitor);

                parser_.restart();
                while (!parser_.stopped()) {
                    parser_.parse(cursor_handler_adaptor_, ec);
                    if (ec) {
                        return;
                    }
                }
            }
        }
    };

    using cbor_stream_cursor = basic_cbor_cursor<turbo::binary_stream_source>;
    using cbor_bytes_cursor = basic_cbor_cursor<turbo::bytes_source>;

} // namespace turbo::cbor

#endif  // TURBO_JSONCONS_CBOR_CBOR_CURSOR_H_

