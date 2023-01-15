
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_STRINGS_UTF8_GREEDY_TABLE_DECODER_H_
#define TURBO_STRINGS_UTF8_GREEDY_TABLE_DECODER_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include "turbo/base/profile.h"

namespace turbo::utf8_detail {

    class greedy_table_decoder {
    public:
        using char8_t = unsigned char;
        using ptrdiff_t = std::ptrdiff_t;

    public:
        static size_t count_unit_size(char8_t const *pSrc, char8_t const *pSrcEnd) noexcept;

        // Conversion to UTF-32 using greedy lookup table
        static ptrdiff_t convert(char8_t const *pSrc, char8_t const *pSrcEnd, char32_t *pDst) noexcept;

    private:
        struct first_unit_info {
            char8_t mFirstOctet;
            std::uint8_t size;
        };
        struct alignas(64) lookup_tables {
            first_unit_info maFirstUnitTable[128];
        };

    private:
        static lookup_tables const smTables;

    private:

        static void skip_by_greedy_2(char8_t const *&pSrc) noexcept;

        static void advance_by_greedy_2(char8_t const *&pSrc, char32_t &cdpt) noexcept;

        static void skip_by_greedy_3(char8_t const *&pSrc) noexcept;

        static void advance_by_greedy_3(char8_t const *&pSrc, char32_t &cdpt) noexcept;

        static void skip_by_greedy_4(char8_t const *&pSrc) noexcept;

        static void advance_by_greedy_4(char8_t const *&pSrc, char32_t &cdpt) noexcept;
    };

    TURBO_FORCE_INLINE void greedy_table_decoder::skip_by_greedy_2(const char8_t *&pSrc) noexcept {
        if ((pSrc[0] & 0xC0) == 0x80) {
            pSrc += 1;
        }
    }

    TURBO_FORCE_INLINE void greedy_table_decoder::advance_by_greedy_2(const char8_t *&pSrc,
                                                                  char32_t &cdpt) noexcept {
        char32_t unit2;   //- The second UTF-8 code unit
        unit2 = pSrc[0];  //- Cache the second code unit

        //- Compute code point
        if ((unit2 & 0xC0) != 0x80) {
            cdpt = 0xFFFD;
        } else {
            cdpt = (cdpt << 6) | (unit2 & 0x3F);
            pSrc += 1;
        }
    }

    TURBO_FORCE_INLINE void greedy_table_decoder::skip_by_greedy_3(const char8_t *&pSrc) noexcept {
        if (((pSrc[0] & 0xC0) == 0x80) && ((pSrc[1] & 0xC0) == 0x80)) {
            pSrc += 2;
        }
    }

    TURBO_FORCE_INLINE void greedy_table_decoder::advance_by_greedy_3(const char8_t *&pSrc,
                                                                  char32_t &cdpt) noexcept {
        char32_t unit2;  //- The second UTF-8 code unit
        char32_t unit3;  //- The third UTF-8 code unit

        unit2 = pSrc[0];  //- Cache the second code unit
        unit3 = pSrc[1];  //- Cache the third code unit

        //- Compute code point
        if ((unit2 & 0xC0) != 0x80 || (unit3 & 0xC0) != 0x80) {
            cdpt = 0xFFFD;
        } else {
            cdpt = (cdpt << 12) | ((unit2 & 0x3F) << 6) | (unit3 & 0x3F);
            pSrc += 2;
        }
    }

    TURBO_FORCE_INLINE void greedy_table_decoder::skip_by_greedy_4(const char8_t *&pSrc) noexcept {
        if (((pSrc[0] & 0xC0) == 0x80) && ((pSrc[1] & 0xC0) == 0x80) && ((pSrc[2] & 0xC0) == 0x80)) {
            pSrc += 3;
        }
    }

    TURBO_FORCE_INLINE void greedy_table_decoder::advance_by_greedy_4(const char8_t *&pSrc,
                                                                  char32_t &cdpt) noexcept {
        char32_t unit2;  //- The second UTF-8 code unit
        char32_t unit3;  //- The third UTF-8 code unit
        char32_t unit4;  //- The fourth UTF-8 code unit

        unit2 = pSrc[0];  //- Cache the second code unit
        unit3 = pSrc[1];  //- Cache the third code unit
        unit4 = pSrc[2];  //- Cache the fourth code unit

        //- Compute code point
        if ((unit2 & 0xC0) != 0x80 || (unit3 & 0xC0) != 0x80 || (unit4 & 0xC0) != 0x80) {
            cdpt = 0xFFFD;
        } else {
            cdpt = (cdpt << 18) | ((unit2 & 0x3F) << 12) | ((unit3 & 0x3F) << 6) | (unit4 & 0x3F);
            pSrc += 3;
        }
    }
}  // namespace turbo::utf8_detail
#endif  // TURBO_STRINGS_UTF8_GREEDY_TABLE_DECODER_H_
