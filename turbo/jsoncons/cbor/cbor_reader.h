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

#ifndef TURBO_JSONCONS_CBOR_CBOR_READER_H_
#define TURBO_JSONCONS_CBOR_CBOR_READER_H_

#include <string>
#include <vector>
#include <memory>
#include <utility> // std::move
#include "turbo/jsoncons/json.h"
#include "turbo/jsoncons/source.h"
#include "turbo/jsoncons/config/jsoncons_config.h"
#include "turbo/jsoncons/cbor/cbor_encoder.h"
#include "turbo/jsoncons/cbor/cbor_error.h"
#include "turbo/jsoncons/cbor/cbor_detail.h"
#include "turbo/jsoncons/cbor/cbor_parser.h"

namespace turbo::cbor {

    template<class Source, class Allocator=std::allocator<char>>
    class basic_cbor_reader {
        using char_type = char;

        basic_cbor_parser<Source, Allocator> parser_;
        basic_item_event_visitor_to_json_visitor<char_type, Allocator> adaptor_;
        item_event_visitor &visitor_;
    public:
        template<class Sourceable>
        basic_cbor_reader(Sourceable &&source,
                          json_visitor &visitor,
                          const Allocator alloc)
                : basic_cbor_reader(std::forward<Sourceable>(source),
                                    visitor,
                                    cbor_decode_options(),
                                    alloc) {
        }

        template<class Sourceable>
        basic_cbor_reader(Sourceable &&source,
                          json_visitor &visitor,
                          const cbor_decode_options &options = cbor_decode_options(),
                          const Allocator alloc = Allocator())
                : parser_(std::forward<Sourceable>(source), options, alloc),
                  adaptor_(visitor, alloc), visitor_(adaptor_) {
        }

        template<class Sourceable>
        basic_cbor_reader(Sourceable &&source,
                          item_event_visitor &visitor,
                          const Allocator alloc)
                : basic_cbor_reader(std::forward<Sourceable>(source),
                                    visitor,
                                    cbor_decode_options(),
                                    alloc) {
        }

        template<class Sourceable>
        basic_cbor_reader(Sourceable &&source,
                          item_event_visitor &visitor,
                          const cbor_decode_options &options = cbor_decode_options(),
                          const Allocator alloc = Allocator())
                : parser_(std::forward<Sourceable>(source), options, alloc),
                  visitor_(visitor) {
        }

        void read() {
            std::error_code ec;
            read(ec);
            if (ec) {
                JSONCONS_THROW(ser_error(ec, line(), column()));
            }
        }

        void read(std::error_code &ec) {
            parser_.reset();
            parser_.parse(visitor_, ec);
            if (ec) {
                return;
            }
        }

        std::size_t line() const {
            return parser_.line();
        }

        std::size_t column() const {
            return parser_.column();
        }
    };

    using cbor_stream_reader = basic_cbor_reader<turbo::binary_stream_source>;

    using cbor_bytes_reader = basic_cbor_reader<turbo::bytes_source>;

}  // namespace turbo::cbor

#endif  // TURBO_JSONCONS_CBOR_CBOR_READER_H_

