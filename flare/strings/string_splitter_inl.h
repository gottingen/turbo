
#include <limits.h>

#ifndef FLARE_STRINGS_STRING_SPLITTER_INL_H_
#define FLARE_STRINGS_STRING_SPLITTER_INL_H_

namespace flare {

    StringSplitter::StringSplitter(const char *str_begin,
                                   const char *str_end,
                                   const char sep,
                                   EmptyFieldAction action)
            : _head(str_begin), _str_tail(str_end), _sep(sep), _empty_field_action(action) {
        init();
    }

    StringSplitter::StringSplitter(const char *str, char sep,
                                   EmptyFieldAction action)
            : StringSplitter(str, nullptr, sep, action) {}

    StringSplitter::StringSplitter(const std::string_view &input, char sep,
                                   EmptyFieldAction action)
            : StringSplitter(input.data(), input.data() + input.length(), sep, action) {}

    void StringSplitter::init() {
        // Find the starting _head and _tail.
        if (__builtin_expect(_head != nullptr, 1)) {
            if (_empty_field_action == SKIP_EMPTY_FIELD) {
                for (; _sep == *_head && not_end(_head); ++_head) {}
            }
            for (_tail = _head; *_tail != _sep && not_end(_tail); ++_tail) {}
        } else {
            _tail = nullptr;
        }
    }

    StringSplitter &StringSplitter::operator++() {
        if (__builtin_expect(_tail != nullptr, 1)) {
            if (not_end(_tail)) {
                ++_tail;
                if (_empty_field_action == SKIP_EMPTY_FIELD) {
                    for (; _sep == *_tail && not_end(_tail); ++_tail) {}
                }
            }
            _head = _tail;
            for (; *_tail != _sep && not_end(_tail); ++_tail) {}
        }
        return *this;
    }

    StringSplitter StringSplitter::operator++(int) {
        StringSplitter tmp = *this;
        operator++();
        return tmp;
    }

    StringSplitter::operator const void *() const {
        return (_head != nullptr && not_end(_head)) ? _head : nullptr;
    }

    const char *StringSplitter::field() const {
        return _head;
    }

    size_t StringSplitter::length() const {
        return static_cast<size_t>(_tail - _head);
    }

    std::string_view StringSplitter::field_sp() const {
        return std::string_view(field(), length());
    }

    bool StringSplitter::not_end(const char *p) const {
        return (_str_tail == nullptr) ? *p : (p != _str_tail);
    }

    int StringSplitter::to_int8(int8_t *pv) const {
        long v = 0;
        if (to_long(&v) == 0 && v >= -128 && v <= 127) {
            *pv = (int8_t) v;
            return 0;
        }
        return -1;
    }

    int StringSplitter::to_uint8(uint8_t *pv) const {
        unsigned long v = 0;
        if (to_ulong(&v) == 0 && v <= 255) {
            *pv = (uint8_t) v;
            return 0;
        }
        return -1;
    }

    int StringSplitter::to_int(int *pv) const {
        long v = 0;
        if (to_long(&v) == 0 && v >= INT_MIN && v <= INT_MAX) {
            *pv = (int) v;
            return 0;
        }
        return -1;
    }

    int StringSplitter::to_uint(unsigned int *pv) const {
        unsigned long v = 0;
        if (to_ulong(&v) == 0 && v <= UINT_MAX) {
            *pv = (unsigned int) v;
            return 0;
        }
        return -1;
    }

    int StringSplitter::to_long(long *pv) const {
        char *endptr = nullptr;
        *pv = strtol(field(), &endptr, 10);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringSplitter::to_ulong(unsigned long *pv) const {
        char *endptr = nullptr;
        *pv = strtoul(field(), &endptr, 10);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringSplitter::to_longlong(long long *pv) const {
        char *endptr = nullptr;
        *pv = strtoll(field(), &endptr, 10);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringSplitter::to_ulonglong(unsigned long long *pv) const {
        char *endptr = nullptr;
        *pv = strtoull(field(), &endptr, 10);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringSplitter::to_float(float *pv) const {
        char *endptr = nullptr;
        *pv = strtof(field(), &endptr);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringSplitter::to_double(double *pv) const {
        char *endptr = nullptr;
        *pv = strtod(field(), &endptr);
        return (endptr == field() + length()) ? 0 : -1;
    }

    StringMultiSplitter::StringMultiSplitter(
            const char *str, const char *seps, EmptyFieldAction action)
            : _head(str), _str_tail(nullptr), _seps(seps), _empty_field_action(action) {
        init();
    }

    StringMultiSplitter::StringMultiSplitter(
            const char *str_begin, const char *str_end,
            const char *seps, EmptyFieldAction action)
            : _head(str_begin), _str_tail(str_end), _seps(seps), _empty_field_action(action) {
        init();
    }

    void StringMultiSplitter::init() {
        if (__builtin_expect(_head != nullptr, 1)) {
            if (_empty_field_action == SKIP_EMPTY_FIELD) {
                for (; is_sep(*_head) && not_end(_head); ++_head) {}
            }
            for (_tail = _head; !is_sep(*_tail) && not_end(_tail); ++_tail) {}
        } else {
            _tail = nullptr;
        }
    }

    StringMultiSplitter &StringMultiSplitter::operator++() {
        if (__builtin_expect(_tail != nullptr, 1)) {
            if (not_end(_tail)) {
                ++_tail;
                if (_empty_field_action == SKIP_EMPTY_FIELD) {
                    for (; is_sep(*_tail) && not_end(_tail); ++_tail) {}
                }
            }
            _head = _tail;
            for (; !is_sep(*_tail) && not_end(_tail); ++_tail) {}
        }
        return *this;
    }

    StringMultiSplitter StringMultiSplitter::operator++(int) {
        StringMultiSplitter tmp = *this;
        operator++();
        return tmp;
    }

    bool StringMultiSplitter::is_sep(char c) const {
        for (const char *p = _seps; *p != '\0'; ++p) {
            if (c == *p) {
                return true;
            }
        }
        return false;
    }

    StringMultiSplitter::operator const void *() const {
        return (_head != nullptr && not_end(_head)) ? _head : nullptr;
    }

    const char *StringMultiSplitter::field() const {
        return _head;
    }

    size_t StringMultiSplitter::length() const {
        return static_cast<size_t>(_tail - _head);
    }

    std::string_view StringMultiSplitter::field_sp() const {
        return std::string_view(field(), length());
    }

    bool StringMultiSplitter::not_end(const char *p) const {
        return (_str_tail == nullptr) ? *p : (p != _str_tail);
    }

    int StringMultiSplitter::to_int8(int8_t *pv) const {
        long v = 0;
        if (to_long(&v) == 0 && v >= -128 && v <= 127) {
            *pv = (int8_t) v;
            return 0;
        }
        return -1;
    }

    int StringMultiSplitter::to_uint8(uint8_t *pv) const {
        unsigned long v = 0;
        if (to_ulong(&v) == 0 && v <= 255) {
            *pv = (uint8_t) v;
            return 0;
        }
        return -1;
    }

    int StringMultiSplitter::to_int(int *pv) const {
        long v = 0;
        if (to_long(&v) == 0 && v >= INT_MIN && v <= INT_MAX) {
            *pv = (int) v;
            return 0;
        }
        return -1;
    }

    int StringMultiSplitter::to_uint(unsigned int *pv) const {
        unsigned long v = 0;
        if (to_ulong(&v) == 0 && v <= UINT_MAX) {
            *pv = (unsigned int) v;
            return 0;
        }
        return -1;
    }

    int StringMultiSplitter::to_long(long *pv) const {
        char *endptr = nullptr;
        *pv = strtol(field(), &endptr, 10);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringMultiSplitter::to_ulong(unsigned long *pv) const {
        char *endptr = nullptr;
        *pv = strtoul(field(), &endptr, 10);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringMultiSplitter::to_longlong(long long *pv) const {
        char *endptr = nullptr;
        *pv = strtoll(field(), &endptr, 10);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringMultiSplitter::to_ulonglong(unsigned long long *pv) const {
        char *endptr = nullptr;
        *pv = strtoull(field(), &endptr, 10);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringMultiSplitter::to_float(float *pv) const {
        char *endptr = nullptr;
        *pv = strtof(field(), &endptr);
        return (endptr == field() + length()) ? 0 : -1;
    }

    int StringMultiSplitter::to_double(double *pv) const {
        char *endptr = nullptr;
        *pv = strtod(field(), &endptr);
        return (endptr == field() + length()) ? 0 : -1;
    }

    void KeyValuePairsSplitter::UpdateDelimiterPosition() {
        const std::string_view key_value_pair(key_and_value());
        _delim_pos = key_value_pair.find(_key_value_delim);
        if (_delim_pos == std::string_view::npos) {
            _delim_pos = key_value_pair.length();
        }
    }

}  // namespace flare

#endif  // FLARE_STRINGS_STRING_SPLITTER_INL_H_
