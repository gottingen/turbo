
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_BASE_UUID_H_
#define FLARE_BASE_UUID_H_


#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include "flare/log/logging.h"

namespace flare {

    class uuid {
    public:
        constexpr uuid() = default;

        // If `from` is malformed, the program crashes.
        //
        // To parse UUID from untrusted source, use `TryParse<Uuid>(...)` instead.
        explicit uuid(const std::string_view &from);
        uuid(uint64_t hi, uint64_t lo);

        std::string to_string() const;

        bool operator==(const uuid &other) const noexcept;

        bool operator!=(const uuid &other) const noexcept;

        bool operator<(const uuid &other) const noexcept;

        static uuid generate();

    private:
        int compare(const uuid &other) const noexcept;

        static int32_t to_decimal(char x);

        static uint8_t to_uint8(const char *starts_at);

    private:
        uint8_t _bytes[16] = {0};
    };


    FLARE_FORCE_INLINE uuid::uuid(const std::string_view &from) {
        FLARE_CHECK_EQ(from.size(), 36ul);  // 8-4-4-4-12
        auto p = from.data();

        _bytes[0] = to_uint8(p);
        _bytes[1] = to_uint8(p + 2);
        _bytes[2] = to_uint8(p + 4);
        _bytes[3] = to_uint8(p + 6);
        p += 8;
        FLARE_CHECK_EQ(*p++, '-');

        _bytes[4] = to_uint8(p);
        _bytes[5] = to_uint8(p + 2);
        p += 4;
        FLARE_CHECK_EQ(*p++, '-');

        _bytes[6] = to_uint8(p);
        _bytes[7] = to_uint8(p + 2);
        p += 4;
        FLARE_CHECK_EQ(*p++, '-');

        _bytes[8] = to_uint8(p);
        _bytes[9] = to_uint8(p + 2);
        p += 4;
        FLARE_CHECK_EQ(*p++, '-');

        _bytes[10] = to_uint8(p);
        _bytes[11] = to_uint8(p + 2);
        _bytes[12] = to_uint8(p + 4);
        _bytes[13] = to_uint8(p + 6);
        _bytes[14] = to_uint8(p + 8);
        _bytes[15] = to_uint8(p + 10);
    }

    FLARE_FORCE_INLINE uuid::uuid(uint64_t hi, uint64_t lo) {
        uint64_t* ptr =(uint64_t*)_bytes;
        *ptr++ = hi;
        *ptr = lo;
    }

    FLARE_FORCE_INLINE bool uuid::operator==(const uuid &other) const noexcept {
        return compare(other) == 0;
    }

    FLARE_FORCE_INLINE bool uuid::operator!=(const uuid &other) const noexcept {
        return compare(other) != 0;
    }

    FLARE_FORCE_INLINE bool uuid::operator<(const uuid &other) const noexcept {
        return compare(other) < 0;
    }

    FLARE_FORCE_INLINE int uuid::compare(const uuid &other) const noexcept {
        // `memcmp` is not `constexpr`..
        return __builtin_memcmp(_bytes, other._bytes, sizeof(_bytes));
    }

    FLARE_FORCE_INLINE int32_t uuid::to_decimal(char x) {
        if (x >= '0' && x <= '9') {
            return x - '0';
        } else if (x >= 'a' && x <= 'f') {
            return x - 'a' + 10;
        } else if (x >= 'A' && x <= 'F') {
            return x - 'A' + 10;
        } else {
            FLARE_CHECK(0)<<"Invalid hex digit ["<<x<<"].";
        }
        return 0;
    }

    FLARE_FORCE_INLINE uint8_t uuid::to_uint8(const char *starts_at) {  // `ToUint8`?
        return to_decimal(starts_at[0]) * 16 + to_decimal(starts_at[1]);
    }


   bool parse_uuid(const std::string_view &s, uuid *ret);

}  // namespace flare

#endif // FLARE_BASE_UUID_H_
