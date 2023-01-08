
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_STRINGS_BYTE_SET_H_
#define FLARE_STRINGS_BYTE_SET_H_

#include <cctype>
#include <climits>
#include <string_view>

namespace flare {


    // Implements a set of byte.
    class byte_set {
    public:
        using value_type = std::uint8_t;  // `unsigned char`.

        constexpr byte_set() : _rep{} {}

        constexpr explicit byte_set(const char *str) : _rep(from_string(str)) {}

        constexpr explicit byte_set(const std::string_view &bytes) : byte_set() {
            insert(bytes);
        }

        // fill with pred
        template<typename Pred>
        explicit byte_set(const Pred &pred) : byte_set() {
            insert_if(pred);
        }

        constexpr byte_set operator|(const byte_set &bs) const {
            return byte_set{_rep | bs._rep};
        }

        constexpr byte_set operator|(const char *s) const {
            return *this | byte_set(s);
        }

        constexpr byte_set operator&(const byte_set &bs) const {
            return byte_set{_rep & bs._rep};
        }

        constexpr bool operator==(const byte_set &bs) const { return _rep == bs._rep; }

        // insert one byte
        void insert(value_type n) { _rep.u[n / 64] |= (1ULL << (n % 64)); }

        // remove one byte
        void erase(value_type n) { _rep.u[n / 64] &= ~(1ULL << (n % 64)); }

        // clear all bytes
        void clear() { _rep = {}; }

        // insert bytes by pred
        // bool pred(value_type byte)
        template<typename Pred>
        void insert_if(const Pred &pred) {
            for (int c = 0; c <= UCHAR_MAX; ++c) {
                if (pred(c)) insert(c);
            }
        }

        // insert any bytes in str into set
        void insert(const std::string_view &bytes) {
            for (auto &&e : bytes) {
                insert(e);
            }
        }

        // remove all bytes which satisfy the pred
        template<typename Pred>
        void erase_if(const Pred &pred) {
            for (int c = 0; c <= UCHAR_MAX; ++c) {
                if (pred(c)) erase(c);
            }
        }

        // remove all bytes in str from the set
        void erase(const std::string_view &bytes) {
            for (auto &&e : bytes) {
                erase(e);
            }
        }

        // return whether exist
        constexpr bool contains(value_type c) const {
            return (_rep.u[c / 64] >> (c % 64)) & 1;
        }

        // OR the set with rhs
        byte_set &operator|=(const byte_set &rhs) {
            _rep |= rhs._rep;
            return *this;
        }

        // AND the set with rhs
        byte_set &operator&=(const byte_set &rhs) {
            _rep &= rhs._rep;
            return *this;
        }

        // OR the set with string
        byte_set &operator|=(const char *rhs) {
            insert(rhs);
            return *this;
        }

    public:  // predefined singletons
        // return the set of all bytes satisfy isspace
        static const byte_set &spaces() {
            static const byte_set cs(isspace);
            return cs;
        }

        // return the set of all bytes satisfy isblank
        static const byte_set &blanks() {
            static const byte_set cs(isblank);
            return cs;
        }

        // return the set of all bytes satisfy isspace
        static const byte_set &alphas() {
            static const byte_set cs(isalpha);
            return cs;
        }

        // return the set of all bytes satisfy isalnum
        static const byte_set &alpha_nums() {
            static const byte_set cs(isalnum);
            return cs;
        }

        // return the set of all bytes satisfy isascii
        static const byte_set &asciis() {
            static const byte_set cs(isascii);
            return cs;
        }

        // return the set of all bytes satisfy isxdigit
        static const byte_set &hex() {
            static const byte_set cs(isxdigit);
            return cs;
        }

        // return the set of all bytes satisfy isdigit
        static const byte_set &digits() {
            static const byte_set cs(isdigit);
            return cs;
        }

        // return the set of all bytes satisfy isupper
        static const byte_set &uppercase() {
            static const byte_set cs(isupper);
            return cs;
        }

        // return the set of all bytes satisfy islower
        static const byte_set &lowercase() {
            static const byte_set cs(islower);
            return cs;
        }

        // return the set of all bytes satisfy isprint
        static const byte_set &printables() {
            static const byte_set cs(isprint);
            return cs;
        }

    private:
        struct rep_impl {
            uint64_t u[4];

            constexpr rep_impl operator|(const rep_impl &rep) const {
                return {
                        {u[0] | rep.u[0], u[1] | rep.u[1], u[2] | rep.u[2], u[3] | rep.u[3]}};
            }

            constexpr rep_impl operator&(const rep_impl &rep) const {
                return {
                        {u[0] & rep.u[0], u[1] & rep.u[1], u[2] & rep.u[2], u[3] & rep.u[3]}};
            }

            rep_impl &operator|=(const rep_impl &rep) {
                u[0] |= rep.u[0];
                u[1] |= rep.u[1];
                u[2] |= rep.u[2];
                u[3] |= rep.u[3];
                return *this;
            }

            rep_impl &operator&=(const rep_impl &rep) {
                u[0] &= rep.u[0];
                u[1] &= rep.u[1];
                u[2] &= rep.u[2];
                u[3] &= rep.u[3];
                return *this;
            }

            constexpr bool operator==(const rep_impl &rep) const {
                return u[0] == rep.u[0] && u[1] == rep.u[1] && u[2] == rep.u[2] &&
                       u[3] == rep.u[3];
            }
        };

        static constexpr rep_impl from_char(value_type c) {
            return c < 64 ? rep_impl{{(1ULL << c), 0, 0, 0}}
                          : c < 128 ? rep_impl{{0, (1ULL << (c - 64)), 0, 0}}
                                    : c < 192 ? rep_impl{{0, 0, (1ULL << (c - 128)), 0}}
                                              : rep_impl{{0, 0, 0, (1ULL << (c - 192))}};
        }

        static constexpr rep_impl from_string(const char *s) {
            return *s == '\0' ? rep_impl() : from_char(*s) | from_string(s + 1);
        }

        explicit constexpr byte_set(rep_impl rep) : _rep(rep) {}

    private:
        rep_impl _rep;
    };


}  // namespace flare

#endif  // FLARE_STRINGS_BYTE_SET_H_
