
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_STRINGS_INTERNAL_CHAR_TRAITS_H_
#define FLARE_STRINGS_INTERNAL_CHAR_TRAITS_H_
#include <cstddef>
#include <cstring>
#include "flare/base/profile.h"  // disable some warnings on Windows
#include "flare/strings/ascii.h"  // for flare::ascii_tolower


namespace flare {
namespace strings_internal {

FLARE_FORCE_INLINE char *char_cat(char *dest, size_t destlen, const char *src,
                                 size_t srclen) {
    return reinterpret_cast<char *>(memcpy(dest + destlen, src, srclen));
}


int char_case_cmp(const char *s1, const char *s2, size_t len);

char *char_dup(const char *s, size_t slen);

char *char_rchr(const char *s, int c, size_t slen);

size_t char_spn(const char *s, size_t slen, const char *accept);

size_t char_cspn(const char *s, size_t slen, const char *reject);

char *char_pbrk(const char *s, size_t slen, const char *accept);

// This is for internal use only.  Don't call this directly
template<bool case_sensitive>
const char *int_char_match(const char *haystack, size_t haylen,
                           const char *needle, size_t neelen) {
    if (0 == neelen) {
        return haystack;  // even if haylen is 0
    }
    const char *hayend = haystack + haylen;
    const char *needlestart = needle;
    const char *needleend = needlestart + neelen;

    for (; haystack < hayend; ++haystack) {
        char hay = case_sensitive
                   ? *haystack
                   : flare::ascii::to_lower(static_cast<unsigned char>(*haystack));
        char nee = case_sensitive
                   ? *needle
                   : flare::ascii::to_lower(static_cast<unsigned char>(*needle));
        if (hay == nee) {
            if (++needle == needleend) {
                return haystack + 1 - neelen;
            }
        } else if (needle != needlestart) {
            // must back up haystack in case a prefix matched (find "aab" in "aaab")
            haystack -= needle - needlestart;  // for loop will advance one more
            needle = needlestart;
        }
    }
    return nullptr;
}

// These are the guys you can call directly
FLARE_FORCE_INLINE const char *char_str(const char *phaystack, size_t haylen,
                                       const char *pneedle) {
    return int_char_match<true>(phaystack, haylen, pneedle, strlen(pneedle));
}

FLARE_FORCE_INLINE const char *char_case_str(const char *phaystack, size_t haylen,
                                            const char *pneedle) {
    return int_char_match<false>(phaystack, haylen, pneedle, strlen(pneedle));
}

FLARE_FORCE_INLINE const char *char_mem(const char *phaystack, size_t haylen,
                                       const char *pneedle, size_t needlelen) {
    return int_char_match<true>(phaystack, haylen, pneedle, needlelen);
}

FLARE_FORCE_INLINE const char *char_case_mem(const char *phaystack, size_t haylen,
                                            const char *pneedle, size_t needlelen) {
    return int_char_match<false>(phaystack, haylen, pneedle, needlelen);
}

// This is significantly faster for case-sensitive matches with very
// few possible matches.  See unit test for benchmarks.
const char *char_match(const char *phaystack, size_t haylen, const char *pneedle,
                       size_t neelen);


}  // namespace strings_internal
}  // namespace flare

#endif  // FLARE_STRINGS_INTERNAL_CHAR_TRAITS_H_
