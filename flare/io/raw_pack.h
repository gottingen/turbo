// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef FLARE_IO_RAW_PACK_H_
#define FLARE_IO_RAW_PACK_H_

#include "flare/base/endian.h"

namespace flare {

    // -------------------------------------------------------------------------
    // NOTE: raw_packer/raw_unpacker is used for packing/unpacking low-level and
    // hard-to-change header. If the fields are likely to be changed in future,
    // use protobuf.
    // -------------------------------------------------------------------------

    // This utility class packs 32-bit and 64-bit integers into binary data
    // that can be unpacked by raw_unpacker. Notice that the packed data is
    // schemaless and user must match pack..() methods with same-width
    // unpack..() methods to get the integers back.
    // Example:
    //   char buf[16];  // 4 + 8 + 4 bytes.
    //   flare::raw_packer(buf).pack32(a).pack64(b).pack32(c);  // buf holds packed data
    //
    //   ... network ...
    //
    //   // positional correspondence with pack..()
    //   flare::raw_unpacker(buf2).unpack32(a).unpack64(b).unpack32(c);
    class raw_packer {
    public:
        // Notice: User must guarantee `stream' is as long as the packed data.
        explicit raw_packer(void *stream) : _stream((char *) stream) {}

        ~raw_packer() {}

        // Not using operator<< because some values may be packed differently from
        // its type.
        raw_packer &pack32(uint32_t host_value) {
            *(uint32_t *) _stream = flare::base::flare_hton32(host_value);
            _stream += 4;
            return *this;
        }

        raw_packer &pack64(uint64_t host_value) {
            uint32_t *p = (uint32_t *) _stream;
            p[0] = flare::base::flare_hton32(host_value >> 32);
            p[1] = flare::base::flare_hton32(host_value & 0xFFFFFFFF);
            _stream += 8;
            return *this;
        }

    private:
        char *_stream;
    };

    // This utility class unpacks 32-bit and 64-bit integers from binary data
    // packed by raw_packer.
    class raw_unpacker {
    public:
        explicit raw_unpacker(const void *stream) : _stream((const char *) stream) {}

        ~raw_unpacker() {}

        raw_unpacker &unpack32(uint32_t &host_value) {
            host_value = flare::base::flare_ntoh32(*(const uint32_t *) _stream);
            _stream += 4;
            return *this;
        }

        raw_unpacker &unpack64(uint64_t &host_value) {
            const uint32_t *p = (const uint32_t *) _stream;
            host_value = (((uint64_t) flare::base::flare_ntoh32(p[0])) << 32) | flare::base::flare_ntoh32(p[1]);
            _stream += 8;
            return *this;
        }

    private:
        const char *_stream;
    };

}  // namespace flare

#endif  // FLARE_IO_RAW_PACK_H_
