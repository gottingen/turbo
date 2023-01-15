
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/strings/utf8/naive_decoder.h"

namespace turbo::utf8_detail {


    static constexpr uint32_t UTF8_MAX = 0x7FFFFFFFu;

    static const unsigned char* utf8_decode_one(const unsigned char* s,
                                              size_t s_limit,
                                              uint32_t* val) noexcept {
        static const uint32_t limits[] = {~0u, 0x80u, 0x800u, 0x10000u, 0x200000u, 0x4000000u};
        unsigned int c = static_cast<unsigned char>(s[0]);
        uint32_t res = 0; /* final result */
        if (c < 0x80) {   /* ascii? */
            res = c;
        } else {
            int count = 0;                /* to count number of continuation bytes */
            for (; c & 0x40u; c <<= 1u) { /* while it needs continuation bytes... */
                if (count >= s_limit) {
                    return nullptr;
                }
                unsigned int cc = (unsigned char)s[++count]; /* read next byte */
                if ((cc & 0xC0u) != 0x80u) {                 /* not a continuation byte? */
                    return nullptr;                            /* invalid byte sequence */
                }
                res = (res << 6u) | (cc & 0x3Fu); /* add lower 6 bits from cont. byte */
            }
            res |= ((uint32_t)(c & 0x7Fu) << (count * 5u)); /* add first byte */
            if (count > 5 || res > UTF8_MAX || res < limits[count]) {
                return nullptr; /* invalid byte sequence */
            }
            s += count; /* skip continuation bytes read */
        }
        if (val) {
            *val = res;
        }
        return s + 1; /* +1 to include first byte */
    }

    ptrdiff_t naive_decoder(unsigned char const* s_ptr,
                           unsigned char const* s_ptr_end,
                           char32_t* dst) noexcept {
        auto* dest_orig = dst;
        const unsigned char* data = s_ptr;
        uint32_t code = 0;
        size_t limit = s_ptr_end - s_ptr;
        while (data && limit > 0) {
            const unsigned char* next_data = utf8_decode_one(data, limit, &code);
            if (next_data) {
                *dst++ = code;
                limit -= (next_data - data);
                data = next_data;
            } else {
                // for invalid utf8, use 0xFFFD instead
                *dst++ = 0xFFFD;
                ++data;
                --limit;
            }
        }
        return dst - dest_orig;
    }
}  // namespace turbo::utf8_detail
