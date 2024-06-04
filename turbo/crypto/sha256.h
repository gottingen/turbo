//
// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// Created by jeff on 24-6-5.
//

#pragma once


#include <array>
#include <cstdint>
#include <turbo/strings/string_view.h>

namespace turbo {

    template <typename T> class ArrayRef;

    class SHA256 {
    public:
        explicit SHA256() { init(); }

        /// Reinitialize the internal state
        void init();

        /// Digest more data.
        void update(ArrayRef<uint8_t> Data);

        /// Digest more data.
        void update(turbo::string_view Str);

        /// Return the current raw 256-bits SHA256 for the digested
        /// data since the last call to init(). This call will add data to the
        /// internal state and as such is not suited for getting an intermediate
        /// result (see result()).
        std::array<uint8_t, 32> final();

        /// Return the current raw 256-bits SHA256 for the digested
        /// data since the last call to init(). This is suitable for getting the
        /// SHA256 at any time without invalidating the internal state so that more
        /// calls can be made into update.
        std::array<uint8_t, 32> result();

        /// Returns a raw 256-bit SHA256 hash for the given data.
        static std::array<uint8_t, 32> hash(ArrayRef<uint8_t> Data);

    private:
        /// Define some constants.
        /// "static constexpr" would be cleaner but MSVC does not support it yet.
        enum { BLOCK_LENGTH = 64 };
        enum { HASH_LENGTH = 32 };

        // Internal State
        struct {
            union {
                uint8_t C[BLOCK_LENGTH];
                uint32_t L[BLOCK_LENGTH / 4];
            } Buffer;
            uint32_t State[HASH_LENGTH / 4];
            uint32_t ByteCount;
            uint8_t BufferOffset;
        } InternalState;

        // Helper
        void writebyte(uint8_t data);
        void hashBlock();
        void addUncounted(uint8_t data);
        void pad();

        void final(std::array<uint32_t, HASH_LENGTH / 4> &HashResult);
    };

} // namespace turbo
