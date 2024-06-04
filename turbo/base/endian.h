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

#pragma once

#include <cstdint>
#include <cstdlib>

#include <turbo/base/casts.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/unaligned_access.h>
#include <turbo/base/nullability.h>
#include <turbo/base/port.h>

namespace turbo {

    inline uint64_t gbswap_64(uint64_t host_int) {
#if TURBO_HAVE_BUILTIN(__builtin_bswap64) || defined(__GNUC__)
        return __builtin_bswap64(host_int);
#elif defined(_MSC_VER)
        return _byteswap_uint64(host_int);
#else
        return (((host_int & uint64_t{0xFF}) << 56) |
                ((host_int & uint64_t{0xFF00}) << 40) |
                ((host_int & uint64_t{0xFF0000}) << 24) |
                ((host_int & uint64_t{0xFF000000}) << 8) |
                ((host_int & uint64_t{0xFF00000000}) >> 8) |
                ((host_int & uint64_t{0xFF0000000000}) >> 24) |
                ((host_int & uint64_t{0xFF000000000000}) >> 40) |
                ((host_int & uint64_t{0xFF00000000000000}) >> 56));
#endif
    }

    inline uint32_t gbswap_32(uint32_t host_int) {
#if TURBO_HAVE_BUILTIN(__builtin_bswap32) || defined(__GNUC__)
        return __builtin_bswap32(host_int);
#elif defined(_MSC_VER)
        return _byteswap_ulong(host_int);
#else
        return (((host_int & uint32_t{0xFF}) << 24) |
                ((host_int & uint32_t{0xFF00}) << 8) |
                ((host_int & uint32_t{0xFF0000}) >> 8) |
                ((host_int & uint32_t{0xFF000000}) >> 24));
#endif
    }

    inline uint16_t gbswap_16(uint16_t host_int) {
#if TURBO_HAVE_BUILTIN(__builtin_bswap16) || defined(__GNUC__)
        return __builtin_bswap16(host_int);
#elif defined(_MSC_VER)
        return _byteswap_ushort(host_int);
#else
        return (((host_int & uint16_t{0xFF}) << 8) |
                ((host_int & uint16_t{0xFF00}) >> 8));
#endif
    }

#ifdef TURBO_IS_LITTLE_ENDIAN
    static constexpr bool is_little_endian() { return true; }
    // Portable definitions for htonl (host-to-network) and friends on little-endian
    // architectures.
    inline uint16_t ghtons(uint16_t x) { return gbswap_16(x); }

    inline uint32_t ghtonl(uint32_t x) { return gbswap_32(x); }

    inline uint64_t ghtonll(uint64_t x) { return gbswap_64(x); }

#elif defined TURBO_IS_BIG_ENDIAN
    static constexpr bool is_little_endian() { return false; }

    // Portable definitions for htonl (host-to-network) etc on big-endian
    // architectures. These definitions are simpler since the host byte order is the
    // same as network byte order.
    inline uint16_t ghtons(uint16_t x) { return x; }
    inline uint32_t ghtonl(uint32_t x) { return x; }
    inline uint64_t ghtonll(uint64_t x) { return x; }

#else
#error \
    "Unsupported byte order: Either TURBO_IS_BIG_ENDIAN or " \
       "TURBO_IS_LITTLE_ENDIAN must be defined"
#endif  // byte order

    inline uint16_t gntohs(uint16_t x) { return ghtons(x); }

    inline uint32_t gntohl(uint32_t x) { return ghtonl(x); }

    inline uint64_t gntohll(uint64_t x) { return ghtonll(x); }

    // Utilities to convert numbers between the current hosts's native byte
    // order and little-endian byte order
    //
    // Load/Store methods are alignment safe
    namespace little_endian {
        // Conversion functions.
#ifdef TURBO_IS_LITTLE_ENDIAN

        inline uint16_t from_host16(uint16_t x) { return x; }

        inline uint16_t to_host16(uint16_t x) { return x; }

        inline uint32_t from_host32(uint32_t x) { return x; }

        inline uint32_t to_host32(uint32_t x) { return x; }

        inline uint64_t from_host64(uint64_t x) { return x; }

        inline uint64_t to_host64(uint64_t x) { return x; }

        inline constexpr bool is_little_endian() { return true; }

#elif defined TURBO_IS_BIG_ENDIAN

        inline uint16_t from_host16(uint16_t x) { return gbswap_16(x); }
        inline uint16_t to_host16(uint16_t x) { return gbswap_16(x); }

        inline uint32_t from_host32(uint32_t x) { return gbswap_32(x); }
        inline uint32_t to_host32(uint32_t x) { return gbswap_32(x); }

        inline uint64_t from_host64(uint64_t x) { return gbswap_64(x); }
        inline uint64_t to_host64(uint64_t x) { return gbswap_64(x); }

        inline constexpr bool is_little_endian() { return false; }

#endif /* ENDIAN */

        inline uint8_t from_host(uint8_t x) { return x; }

        inline uint16_t from_host(uint16_t x) { return from_host16(x); }

        inline uint32_t from_host(uint32_t x) { return from_host32(x); }

        inline uint64_t from_host(uint64_t x) { return from_host64(x); }

        inline uint8_t to_host(uint8_t x) { return x; }

        inline uint16_t to_host(uint16_t x) { return to_host16(x); }

        inline uint32_t to_host(uint32_t x) { return to_host32(x); }

        inline uint64_t to_host(uint64_t x) { return to_host64(x); }

        inline int8_t from_host(int8_t x) { return x; }

        inline int16_t from_host(int16_t x) {
            return bit_cast<int16_t>(from_host16(bit_cast<uint16_t>(x)));
        }

        inline int32_t from_host(int32_t x) {
            return bit_cast<int32_t>(from_host32(bit_cast<uint32_t>(x)));
        }

        inline int64_t from_host(int64_t x) {
            return bit_cast<int64_t>(from_host64(bit_cast<uint64_t>(x)));
        }

        inline int8_t to_host(int8_t x) { return x; }

        inline int16_t to_host(int16_t x) {
            return bit_cast<int16_t>(to_host16(bit_cast<uint16_t>(x)));
        }

        inline int32_t to_host(int32_t x) {
            return bit_cast<int32_t>(to_host32(bit_cast<uint32_t>(x)));
        }

        inline int64_t to_host(int64_t x) {
            return bit_cast<int64_t>(to_host64(bit_cast<uint64_t>(x)));
        }

        // Functions to do unaligned loads and stores in little-endian order.
        inline uint16_t load16(turbo::Nonnull<const void *> p) {
            return to_host16(TURBO_INTERNAL_UNALIGNED_LOAD16(p));
        }

        inline void store16(turbo::Nonnull<void *> p, uint16_t v) {
            TURBO_INTERNAL_UNALIGNED_STORE16(p, from_host16(v));
        }

        inline uint32_t load32(turbo::Nonnull<const void *> p) {
            return to_host32(TURBO_INTERNAL_UNALIGNED_LOAD32(p));
        }

        inline void store32(turbo::Nonnull<void *> p, uint32_t v) {
            TURBO_INTERNAL_UNALIGNED_STORE32(p, from_host32(v));
        }

        inline uint64_t load64(turbo::Nonnull<const void *> p) {
            return to_host64(TURBO_INTERNAL_UNALIGNED_LOAD64(p));
        }

        inline void store64(turbo::Nonnull<void *> p, uint64_t v) {
            TURBO_INTERNAL_UNALIGNED_STORE64(p, from_host64(v));
        }

    }  // namespace little_endian

    // Utilities to convert numbers between the current hosts's native byte
    // order and big-endian byte order (same as network byte order)
    //
    // Load/Store methods are alignment safe
    namespace big_endian {
#ifdef TURBO_IS_LITTLE_ENDIAN

        inline uint16_t from_host16(uint16_t x) { return gbswap_16(x); }

        inline uint16_t to_host16(uint16_t x) { return gbswap_16(x); }

        inline uint32_t from_host32(uint32_t x) { return gbswap_32(x); }

        inline uint32_t to_host32(uint32_t x) { return gbswap_32(x); }

        inline uint64_t from_host64(uint64_t x) { return gbswap_64(x); }

        inline uint64_t to_host64(uint64_t x) { return gbswap_64(x); }

        inline constexpr bool is_little_endian() { return true; }

#elif defined TURBO_IS_BIG_ENDIAN

        inline uint16_t from_host16(uint16_t x) { return x; }
        inline uint16_t to_host16(uint16_t x) { return x; }

        inline uint32_t from_host32(uint32_t x) { return x; }
        inline uint32_t to_host32(uint32_t x) { return x; }

        inline uint64_t from_host64(uint64_t x) { return x; }
        inline uint64_t to_host64(uint64_t x) { return x; }

        inline constexpr bool is_little_endian() { return false; }

#endif /* ENDIAN */

        inline uint8_t from_host(uint8_t x) { return x; }

        inline uint16_t from_host(uint16_t x) { return from_host16(x); }

        inline uint32_t from_host(uint32_t x) { return from_host32(x); }

        inline uint64_t from_host(uint64_t x) { return from_host64(x); }

        inline uint8_t to_host(uint8_t x) { return x; }

        inline uint16_t to_host(uint16_t x) { return to_host16(x); }

        inline uint32_t to_host(uint32_t x) { return to_host32(x); }

        inline uint64_t to_host(uint64_t x) { return to_host64(x); }

        inline int8_t from_host(int8_t x) { return x; }

        inline int16_t from_host(int16_t x) {
            return bit_cast<int16_t>(from_host16(bit_cast<uint16_t>(x)));
        }

        inline int32_t from_host(int32_t x) {
            return bit_cast<int32_t>(from_host32(bit_cast<uint32_t>(x)));
        }

        inline int64_t from_host(int64_t x) {
            return bit_cast<int64_t>(from_host64(bit_cast<uint64_t>(x)));
        }

        inline int8_t to_host(int8_t x) { return x; }

        inline int16_t to_host(int16_t x) {
            return bit_cast<int16_t>(to_host16(bit_cast<uint16_t>(x)));
        }

        inline int32_t to_host(int32_t x) {
            return bit_cast<int32_t>(to_host32(bit_cast<uint32_t>(x)));
        }

        inline int64_t to_host(int64_t x) {
            return bit_cast<int64_t>(to_host64(bit_cast<uint64_t>(x)));
        }

        // Functions to do unaligned loads and stores in big-endian order.
        inline uint16_t load16(turbo::Nonnull<const void *> p) {
            return to_host16(TURBO_INTERNAL_UNALIGNED_LOAD16(p));
        }

        inline void store16(turbo::Nonnull<void *> p, uint16_t v) {
            TURBO_INTERNAL_UNALIGNED_STORE16(p, from_host16(v));
        }

        inline uint32_t load32(turbo::Nonnull<const void *> p) {
            return to_host32(TURBO_INTERNAL_UNALIGNED_LOAD32(p));
        }

        inline void store32(turbo::Nonnull<void *> p, uint32_t v) {
            TURBO_INTERNAL_UNALIGNED_STORE32(p, from_host32(v));
        }

        inline uint64_t load64(turbo::Nonnull<const void *> p) {
            return to_host64(TURBO_INTERNAL_UNALIGNED_LOAD64(p));
        }

        inline void store64(turbo::Nonnull<void *> p, uint64_t v) {
            TURBO_INTERNAL_UNALIGNED_STORE64(p, from_host64(v));
        }

    }  // namespace big_endian
#if TURBO_IS_LITTLE_ENDIAN

    inline uint16_t load16(turbo::Nonnull<const void *> p) {
        return little_endian::load16(p);
    }

    inline void store16(turbo::Nonnull<void *> p, uint16_t v) {
        little_endian::store16(p, v);
    }

    inline uint32_t load32(turbo::Nonnull<const void *> p) {
        return little_endian::load32(p);
    }

    inline void store32(turbo::Nonnull<void *> p, uint32_t v) {
        little_endian::store32(p, v);
    }

    inline uint64_t load64(turbo::Nonnull<const void *> p) {
        return little_endian::load64(p);
    }

    inline void store64(turbo::Nonnull<void *> p, uint64_t v) {
        little_endian::store64(p, v);
    }

#elif defined TURBO_IS_BIG_ENDIAN
    inline uint16_t load16(turbo::Nonnull<const void *> p) {
        return big_endian::load16(p);
    }

    inline void store16(turbo::Nonnull<void *> p, uint16_t v) {
        big_endian::store16(p, v);
    }
    inline uint32_t load32(turbo::Nonnull<const void *> p) {
        return big_endian::load32(p);
    }

    inline void store32(turbo::Nonnull<void *> p, uint32_t v) {
        big_endian::store32(p, v);
    }

    inline uint64_t load64(turbo::Nonnull<const void *> p) {
        return big_endian::load64(p);
    }

    inline void store64(turbo::Nonnull<void *> p, uint64_t v) {
            big_endian::store64(p, v);
    }
#else
#error \
    "Unsupported byte order: Either TURBO_IS_BIG_ENDIAN or " \
       "TURBO_IS_LITTLE_ENDIAN must be defined"
#endif  // byte order

}  // namespace turbo

