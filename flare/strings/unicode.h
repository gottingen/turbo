
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_UNICODE_H_
#define FLARE_UNICODE_H_

#include <string_view>
#include "flare/base/profile.h"
#include "flare/base/throw_delegate.h"
#include <string>

namespace flare {

enum utf8_states_t {
    S_STRT = 0, S_RJCT = 8
};

FLARE_FORCE_INLINE bool in_range(uint32_t c, uint32_t lo, uint32_t hi) {
    return (static_cast<uint32_t>(c - lo) < (hi - lo + 1));
}

FLARE_FORCE_INLINE bool is_surrogate(uint32_t c) {
    return in_range(c, 0xd800, 0xdfff);
}

FLARE_FORCE_INLINE bool is_high_surrogate(uint32_t c) {
    return (c & 0xfffffc00) == 0xd800;
}

FLARE_FORCE_INLINE bool is_low_surrogate(uint32_t c) {
    return (c & 0xfffffc00) == 0xdc00;
}

FLARE_FORCE_INLINE void append_utf8(std::string &str, uint32_t unicode) {
    if (unicode <= 0x7f) {
        str.push_back(static_cast<char>(unicode));
    } else if (unicode >= 0x80 && unicode <= 0x7ff) {
        str.push_back(static_cast<char>((unicode >> 6) + 192));
        str.push_back(static_cast<char>((unicode & 0x3f) + 128));
    } else if ((unicode >= 0x800 && unicode <= 0xd7ff) || (unicode >= 0xe000 && unicode <= 0xffff)) {
        str.push_back(static_cast<char>((unicode >> 12) + 224));
        str.push_back(static_cast<char>(((unicode & 0xfff) >> 6) + 128));
        str.push_back(static_cast<char>((unicode & 0x3f) + 128));
    } else if (unicode >= 0x10000 && unicode <= 0x10ffff) {
        str.push_back(static_cast<char>((unicode >> 18) + 240));
        str.push_back(static_cast<char>(((unicode & 0x3ffff) >> 12) + 128));
        str.push_back(static_cast<char>(((unicode & 0xfff) >> 6) + 128));
        str.push_back(static_cast<char>((unicode & 0x3f) + 128));
    } else {
#ifdef FLARE_RAISE_UNICODE_ERRORS
        throw_std_invalid_argument("Illegal code point for unicode character.");
#else
        append_utf8(str, 0xfffd);
#endif
    }
}

// Thanks to Bjoern Hoehrmann (https://bjoern.hoehrmann.de/utf-8/decoder/dfa/)
// and Taylor R Campbell for the ideas to this DFA approach of UTF-8 decoding;
// Generating debugging and shrinking my own DFA from scratch was a day of fun!
FLARE_FORCE_INLINE unsigned
consume_utf8_fragment(const unsigned state, const uint8_t fragment, uint32_t &codepoint) {
    static const uint32_t utf8_state_info[] = {
            // encoded states
            0x11111111u, 0x11111111u, 0x77777777u, 0x77777777u, 0x88888888u, 0x88888888u, 0x88888888u,
            0x88888888u,
            0x22222299u, 0x22222222u, 0x22222222u, 0x22222222u, 0x3333333au, 0x33433333u, 0x9995666bu,
            0x99999999u,
            0x88888880u, 0x22818108u, 0x88888881u, 0x88888882u, 0x88888884u, 0x88888887u, 0x88888886u,
            0x82218108u,
            0x82281108u, 0x88888888u, 0x88888883u, 0x88888885u, 0u, 0u, 0u, 0u,
    };
    uint8_t category =
            fragment < 128 ? 0 : (utf8_state_info[(fragment >> 3) & 0xf] >> ((fragment & 7) << 2)) & 0xf;
    codepoint = (state ? (codepoint << 6) | (fragment & 0x3fu) : (0xffu >> category) & fragment);
    return state == S_RJCT ? static_cast<unsigned>(S_RJCT) : static_cast<unsigned>(
            (utf8_state_info[category + 16] >> (state << 2)) & 0xf);
}

FLARE_FORCE_INLINE bool is_valid_utf8(const std::string &utf8String) {
    std::string::const_iterator iter = utf8String.begin();
    unsigned utf8_state = S_STRT;
    std::uint32_t codepoint = 0;
    while (iter < utf8String.end()) {
        if ((utf8_state = consume_utf8_fragment(utf8_state, static_cast<uint8_t>(*iter++), codepoint)) ==
            S_RJCT) {
            return false;
        }
    }
    if (utf8_state) {
        return false;
    }
    return true;
}


template<class StringType, typename std::enable_if<(sizeof(typename StringType::value_type) ==
                                                    1)>::type * = nullptr>
inline StringType from_utf8(const std::string &utf8String,
                            const typename StringType::allocator_type &alloc = typename StringType::allocator_type()) {
    return StringType(utf8String.begin(), utf8String.end(), alloc);
}

template<class StringType, typename std::enable_if<(sizeof(typename StringType::value_type) ==
                                                    2)>::type * = nullptr>
inline StringType from_utf8(const std::string &utf8String,
                            const typename StringType::allocator_type &alloc = typename StringType::allocator_type()) {
    StringType result(alloc);
    result.reserve(utf8String.length());
    std::string::const_iterator iter = utf8String.begin();
    unsigned utf8_state = S_STRT;
    std::uint32_t codepoint = 0;
    while (iter < utf8String.end()) {
        if ((utf8_state = consume_utf8_fragment(utf8_state, static_cast<uint8_t>(*iter++), codepoint)) ==
            S_STRT) {
            if (codepoint <= 0xffff) {
                result += static_cast<typename StringType::value_type>(codepoint);
            } else {
                codepoint -= 0x10000;
                result += static_cast<typename StringType::value_type>((codepoint >> 10) + 0xd800);
                result += static_cast<typename StringType::value_type>((codepoint & 0x3ff) + 0xdc00);
            }
            codepoint = 0;
        } else if (utf8_state == S_RJCT) {
#ifdef FLARE_RAISE_UNICODE_ERRORS
            throw_std_invalid_argument("Illegal byte sequence for unicode character.");
#else
            result += static_cast<typename StringType::value_type>(0xfffd);
            utf8_state = S_STRT;
            codepoint = 0;
#endif
        }
    }
    if (utf8_state) {
#ifdef FLARE_RAISE_UNICODE_ERRORS
        throw_std_invalid_argument("Illegal byte sequence for unicode character.");
#else
        result += static_cast<typename StringType::value_type>(0xfffd);
#endif
    }
    return result;
}

template<class StringType, typename std::enable_if<(sizeof(typename StringType::value_type) ==
                                                    4)>::type * = nullptr>
inline StringType from_utf8(const std::string &utf8String,
                            const typename StringType::allocator_type &alloc = typename StringType::allocator_type()) {
    StringType result(alloc);
    result.reserve(utf8String.length());
    std::string::const_iterator iter = utf8String.begin();
    unsigned utf8_state = S_STRT;
    std::uint32_t codepoint = 0;
    while (iter < utf8String.end()) {
        if ((utf8_state = consume_utf8_fragment(utf8_state, static_cast<uint8_t>(*iter++), codepoint)) ==
            S_STRT) {
            result += static_cast<typename StringType::value_type>(codepoint);
            codepoint = 0;
        } else if (utf8_state == S_RJCT) {
#ifdef FLARE_RAISE_UNICODE_ERRORS
            throw_std_invalid_argument("Illegal byte sequence for unicode character.");
#else
            result += static_cast<typename StringType::value_type>(0xfffd);
            utf8_state = S_STRT;
            codepoint = 0;
#endif
        }
    }
    if (utf8_state) {
#ifdef FLARE_RAISE_UNICODE_ERRORS
        throw_std_invalid_argument("Illegal byte sequence for unicode character.");
#else
        result += static_cast<typename StringType::value_type>(0xfffd);
#endif
    }
    return result;
}

template<typename charT, typename traits, typename Alloc, typename std::enable_if<(sizeof(charT) == 1),
        int>::type size = 1>
inline std::string to_utf8(const std::basic_string<charT, traits, Alloc> &unicodeString) {
    return std::string(unicodeString.begin(), unicodeString.end());
}

template<typename charT, typename traits, typename Alloc, typename std::enable_if<(sizeof(charT) == 2),
        int>::type size = 2>
inline std::string to_utf8(const std::basic_string<charT, traits, Alloc> &unicodeString) {
    std::string result;
    for (auto iter = unicodeString.begin(); iter != unicodeString.end(); ++iter) {
        char32_t c = *iter;
        if (is_surrogate(c)) {
            ++iter;
            if (iter != unicodeString.end() && is_high_surrogate(c) && is_low_surrogate(*iter)) {
                append_utf8(result, (char32_t(c) << 10) + *iter - 0x35fdc00);
            } else {
#ifdef FLARE_RAISE_UNICODE_ERRORS
                throw_std_invalid_argument("Illegal byte sequence for unicode character.");
#else
                append_utf8(result, 0xfffd);
                if (iter == unicodeString.end()) {
                    break;
                }
#endif
            }
        } else {
            append_utf8(result, c);
        }
    }
    return result;
}

template<typename charT, typename traits, typename Alloc, typename std::enable_if<(sizeof(charT) == 4),
        int>::type size = 4>
inline std::string to_utf8(const std::basic_string<charT, traits, Alloc> &unicodeString) {
    std::string result;
    for (auto c : unicodeString) {
        append_utf8(result, static_cast<uint32_t>(c));
    }
    return result;
}

template<typename charT>
inline std::string to_utf8(const charT *unicodeString) {
    return to_utf8(std::basic_string<charT, std::char_traits<charT>>(unicodeString));
}

template<class Uint16Container>
bool utf8_to_unicode(std::string_view source, Uint16Container &vec) {
    if (source.empty()) {
        return false;
    }
    char ch1, ch2;
    uint16_t tmp;
    vec.clear();
    const char *str = source.data();
    size_t len = source.size();
    for (size_t i = 0; i < len;) {
        if (!(str[i] & 0x80)) { // 0xxxxxxx
            vec.push_back(str[i]);
            i++;
        } else if ((uint8_t) str[i] <= 0xdf && i + 1 < len) { // 110xxxxxx
            ch1 = (str[i] >> 2) & 0x07;
            ch2 = (str[i + 1] & 0x3f) | ((str[i] & 0x03) << 6);
            tmp = (((uint16_t(ch1) & 0x00ff) << 8) | (uint16_t(ch2) & 0x00ff));
            vec.push_back(tmp);
            i += 2;
        } else if ((uint8_t) str[i] <= 0xef && i + 2 < len) {
            ch1 = ((uint8_t) str[i] << 4) | ((str[i + 1] >> 2) & 0x0f);
            ch2 = (((uint8_t) str[i + 1] << 6) & 0xc0) | (str[i + 2] & 0x3f);
            tmp = (((uint16_t(ch1) & 0x00ff) << 8) | (uint16_t(ch2) & 0x00ff));
            vec.push_back(tmp);
            i += 3;
        } else {
            return false;
        }
    }
    return true;
}


template<class Uint32Container>
bool utf8_to_unicode32(std::string_view source, Uint32Container &vec) {
    uint32_t tmp;
    vec.clear();
    size_t size = source.size();
    const char *str = source.data();
    for (size_t i = 0; i < size;) {
        if (!(str[i] & 0x80)) { // 0xxxxxxx
            // 7bit, total 7bit
            tmp = (uint8_t) (str[i]) & 0x7f;
            i++;
        } else if ((uint8_t) str[i] <= 0xdf && i + 1 < size) { // 110xxxxxx
            // 5bit, total 5bit
            tmp = (uint8_t) (str[i]) & 0x1f;

            // 6bit, total 11bit
            tmp <<= 6;
            tmp |= (uint8_t) (str[i + 1]) & 0x3f;
            i += 2;
        } else if ((uint8_t) str[i] <= 0xef && i + 2 < size) { // 1110xxxxxx
            // 4bit, total 4bit
            tmp = (uint8_t) (str[i]) & 0x0f;

            // 6bit, total 10bit
            tmp <<= 6;
            tmp |= (uint8_t) (str[i + 1]) & 0x3f;

            // 6bit, total 16bit
            tmp <<= 6;
            tmp |= (uint8_t) (str[i + 2]) & 0x3f;

            i += 3;
        } else if ((uint8_t) str[i] <= 0xf7 && i + 3 < size) { // 11110xxxx
            // 3bit, total 3bit
            tmp = (uint8_t) (str[i]) & 0x07;

            // 6bit, total 9bit
            tmp <<= 6;
            tmp |= (uint8_t) (str[i + 1]) & 0x3f;

            // 6bit, total 15bit
            tmp <<= 6;
            tmp |= (uint8_t) (str[i + 2]) & 0x3f;

            // 6bit, total 21bit
            tmp <<= 6;
            tmp |= (uint8_t) (str[i + 3]) & 0x3f;

            i += 4;
        } else {
            return false;
        }
        vec.push_back(tmp);
    }
    return true;
}

FLARE_FORCE_INLINE int unicode_to_utf8_bytes(uint32_t ui) {
    if (ui <= 0x7f) {
        return 1;
    } else if (ui <= 0x7ff) {
        return 2;
    } else if (ui <= 0xffff) {
        return 3;
    } else {
        return 4;
    }
}

template<class Uint32ContainerConIter>
void unicode32_to_utf8(Uint32ContainerConIter begin, Uint32ContainerConIter end, std::string &res) {
    res.clear();
    uint32_t ui;
    while (begin != end) {
        ui = *begin;
        if (ui <= 0x7f) {
            res += char(ui);
        } else if (ui <= 0x7ff) {
            res += char(((ui >> 6) & 0x1f) | 0xc0);
            res += char((ui & 0x3f) | 0x80);
        } else if (ui <= 0xffff) {
            res += char(((ui >> 12) & 0x0f) | 0xe0);
            res += char(((ui >> 6) & 0x3f) | 0x80);
            res += char((ui & 0x3f) | 0x80);
        } else {
            res += char(((ui >> 18) & 0x03) | 0xf0);
            res += char(((ui >> 12) & 0x3f) | 0x80);
            res += char(((ui >> 6) & 0x3f) | 0x80);
            res += char((ui & 0x3f) | 0x80);
        }
        begin++;
    }
}

template<class Uint16ContainerConIter>
void unicode_to_utf8(Uint16ContainerConIter begin, Uint16ContainerConIter end, std::string &res) {
    res.clear();
    uint16_t ui;
    while (begin != end) {
        ui = *begin;
        if (ui <= 0x7f) {
            res += char(ui);
        } else if (ui <= 0x7ff) {
            res += char(((ui >> 6) & 0x1f) | 0xc0);
            res += char((ui & 0x3f) | 0x80);
        } else {
            res += char(((ui >> 12) & 0x0f) | 0xe0);
            res += char(((ui >> 6) & 0x3f) | 0x80);
            res += char((ui & 0x3f) | 0x80);
        }
        begin++;
    }
}

constexpr uint16_t char_to_uint16(char high, char low) {
    return (((uint16_t(high) & 0x00ff) << 8) | (uint16_t(low) & 0x00ff));
}

template<class Uint16Container>
bool gbk_trans(std::string_view source, Uint16Container &vec) {
    vec.clear();
    if (source.empty()) {
        return true;
    }
    size_t i = 0;
    while (i < source.size()) {
        if (0 == (source[i] & 0x80)) {
            vec.push_back(uint16_t(source[i]));
            i++;
        } else {
            if (i + 1 < source.size()) { //&& (str[i+1] & 0x80))
                uint16_t tmp = (((uint16_t(source[i]) & 0x00ff) << 8) | (uint16_t(source[i + 1]) & 0x00ff));
                vec.push_back(tmp);
                i += 2;
            } else {
                return false;
            }
        }
    }
    return true;
}


template<class Uint16ContainerConIter>
void gbk_trans(Uint16ContainerConIter begin, Uint16ContainerConIter end, std::string &res) {
    res.clear();
    //pair<char, char> pa;
    char first, second;
    while (begin != end) {
        //pa = uint16ToChar2(*begin);
        first = ((*begin) >> 8) & 0x00ff;
        second = (*begin) & 0x00ff;
        if (first & 0x80) {
            res += first;
            res += second;
        } else {
            res += second;
        }
        begin++;
    }
}


}  // namespace flare
#endif  // FLARE_UNICODE_H_
