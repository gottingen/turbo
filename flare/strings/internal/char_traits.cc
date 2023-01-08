
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/strings/internal/char_traits.h"


#include <cstdlib>

namespace flare {

namespace strings_internal {

int char_case_cmp(const char *s1, const char *s2, size_t len) {
    const unsigned char *us1 = reinterpret_cast<const unsigned char *>(s1);
    const unsigned char *us2 = reinterpret_cast<const unsigned char *>(s2);

    for (size_t i = 0; i < len; i++) {
        const int diff =
                int{static_cast<unsigned char>(flare::ascii::to_lower(us1[i]))} -
                int{static_cast<unsigned char>(flare::ascii::to_lower(us2[i]))};
        if (diff != 0) return diff;
    }
    return 0;
}

char *char_dup(const char *s, size_t slen) {
    void *copy;
    if ((copy = malloc(slen)) == nullptr) return nullptr;
    memcpy(copy, s, slen);
    return reinterpret_cast<char *>(copy);
}

char *char_rchr(const char *s, int c, size_t slen) {
    for (const char *e = s + slen - 1; e >= s; e--) {
        if (*e == c) return const_cast<char *>(e);
    }
    return nullptr;
}

size_t char_spn(const char *s, size_t slen, const char *accept) {
    const char *p = s;
    const char *spanp;
    char c, sc;

    cont:
    c = *p++;
    if (slen-- == 0) return p - 1 - s;
    for (spanp = accept; (sc = *spanp++) != '\0';)
        if (sc == c) goto cont;
    return p - 1 - s;
}

size_t char_cspn(const char *s, size_t slen, const char *reject) {
    const char *p = s;
    const char *spanp;
    char c, sc;

    while (slen-- != 0) {
        c = *p++;
        for (spanp = reject; (sc = *spanp++) != '\0';)
            if (sc == c) return p - 1 - s;
    }
    return p - s;
}

char *char_pbrk(const char *s, size_t slen, const char *accept) {
    const char *scanp;
    int sc;

    for (; slen; ++s, --slen) {
        for (scanp = accept; (sc = *scanp++) != '\0';)
            if (sc == *s) return const_cast<char *>(s);
    }
    return nullptr;
}

// This is significantly faster for case-sensitive matches with very
// few possible matches.  See unit test for benchmarks.
const char *char_match(const char *phaystack, size_t haylen, const char *pneedle,
                       size_t neelen) {
    if (0 == neelen) {
        return phaystack;  // even if haylen is 0
    }
    if (haylen < neelen) return nullptr;

    const char *match;
    const char *hayend = phaystack + haylen - neelen + 1;
    // A static cast is used here to work around the fact that memchr returns
    // a void* on Posix-compliant systems and const void* on Windows.
    while ((match = static_cast<const char *>(
            memchr(phaystack, pneedle[0], hayend - phaystack)))) {
        if (memcmp(match, pneedle, neelen) == 0)
            return match;
        else
            phaystack = match + 1;
    }
    return nullptr;
}

}  // namespace strings_internal

}  // namespace flare
