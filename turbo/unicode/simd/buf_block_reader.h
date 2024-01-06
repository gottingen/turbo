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

#include "turbo/unicode/simd/fwd.h"
#include "turbo/simd/simd.h"
#include "turbo/base/endian.h"


namespace turbo::unicode::simd {

    // Walks through a buffer in block-sized increments, loading the last part with spaces
    template<size_t STEP_SIZE>
    struct buf_block_reader {
    public:
        TURBO_FORCE_INLINE buf_block_reader(const uint8_t *_buf, size_t _len);

        TURBO_FORCE_INLINE size_t block_index();

        TURBO_FORCE_INLINE bool has_full_block() const;

        TURBO_FORCE_INLINE const uint8_t *full_block() const;
        /**
         * Get the last block, padded with spaces.
         *
         * There will always be a last block, with at least 1 byte, unless len == 0 (in which case this
         * function fills the buffer with spaces and returns 0. In particular, if len == STEP_SIZE there
         * will be 0 full_blocks and 1 remainder block with STEP_SIZE bytes and no spaces for padding.
         *
         * @return the number of effective characters in the last block.
         */
        TURBO_FORCE_INLINE size_t get_remainder(uint8_t *dst) const;

        TURBO_FORCE_INLINE void advance();

    private:
        const uint8_t *buf;
        const size_t len;
        const size_t lenminusstep;
        size_t idx;
    };

    // Routines to print masks and text for debugging bitmask operations
    template<typename Engine>
    [[maybe_unused]] static char *format_input_text_64(const uint8_t *text) {
        static char *buf = reinterpret_cast<char *>(malloc(sizeof(simd8x64<uint8_t,Engine>) + 1));
        for (size_t i = 0; i < sizeof(simd8x64<uint8_t,Engine>); i++) {
            buf[i] = int8_t(text[i]) < ' ' ? '_' : int8_t(text[i]);
        }
        buf[sizeof(simd8x64<uint8_t,Engine>)] = '\0';
        return buf;
    }

    // Routines to print masks and text for debugging bitmask operations
    template<typename Engine>
    [[maybe_unused]] static char *format_input_text(const simd8x64<uint8_t,Engine> &in) {
        static char *buf = reinterpret_cast<char *>(malloc(sizeof(simd8x64<uint8_t,Engine>) + 1));
        in.store(reinterpret_cast<uint8_t *>(buf));
        for (size_t i = 0; i < sizeof(simd8x64<uint8_t,Engine>); i++) {
            if (buf[i] < ' ') { buf[i] = '_'; }
        }
        buf[sizeof(simd8x64<uint8_t,Engine>)] = '\0';
        return buf;
    }

    [[maybe_unused]] static char *format_mask(uint64_t mask) {
        static char *buf = reinterpret_cast<char *>(malloc(64 + 1));
        for (size_t i = 0; i < 64; i++) {
            buf[i] = (mask & (size_t(1) << i)) ? 'X' : ' ';
        }
        buf[64] = '\0';
        return buf;
    }

    template<size_t STEP_SIZE>
    TURBO_FORCE_INLINE
    buf_block_reader<STEP_SIZE>::buf_block_reader(const uint8_t *_buf, size_t _len) : buf{_buf}, len{_len},
                                                                                      lenminusstep{len < STEP_SIZE ? 0 :
                                                                                                   len - STEP_SIZE},
                                                                                      idx{0} {}

    template<size_t STEP_SIZE>
    TURBO_FORCE_INLINE size_t buf_block_reader<STEP_SIZE>::block_index() { return idx; }

    template<size_t STEP_SIZE>
    TURBO_FORCE_INLINE bool buf_block_reader<STEP_SIZE>::has_full_block() const {
        return idx < lenminusstep;
    }

    template<size_t STEP_SIZE>
    TURBO_FORCE_INLINE const uint8_t *buf_block_reader<STEP_SIZE>::full_block() const {
        return &buf[idx];
    }

    template<size_t STEP_SIZE>
    TURBO_FORCE_INLINE size_t buf_block_reader<STEP_SIZE>::get_remainder(uint8_t *dst) const {
        if (len == idx) { return 0; } // memcpy(dst, null, 0) will trigger an error with some sanitizers
        std::memset(dst, 0x20,
                    STEP_SIZE); // std::memset STEP_SIZE because it's more efficient to write out 8 or 16 bytes at once.
        std::memcpy(dst, buf + idx, len - idx);
        return len - idx;
    }

    template<size_t STEP_SIZE>
    TURBO_FORCE_INLINE void buf_block_reader<STEP_SIZE>::advance() {
        idx += STEP_SIZE;
    }

} // namespace turbo::unicode
