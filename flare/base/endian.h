
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef FLARE_SYSTEM_ENDIAN_H_
#define FLARE_SYSTEM_ENDIAN_H_

// The following guarantees declaration of the byte swap functions
#ifdef _MSC_VER
#include <cstdlib>  // NOLINT(build/include)
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#elif defined(__GLIBC__)
#include <byteswap.h>  // IWYU pragma: export
#endif

#include <cstdint>
#include "flare/base/profile.h"
#include "flare/base/unaligned_access.h"
#include "flare/base/profile.h"
#include "flare/base/math.h"

namespace flare::base {

#ifdef FLARE_SYSTEM_LITTLE_ENDIAN

// Definitions for ntohl etc. that don't require us to include
// netinet/in.h. We wrap gbswap_32 and gbswap_16 in functions rather
// than just #defining them because in debug mode, gcc doesn't
// correctly handle the (rather involved) definitions of bswap_32.
// gcc guarantees that FLARE_FORCE_INLINE functions are as fast as macros, so
// this isn't a performance hit.
FLARE_FORCE_INLINE uint16_t flare_hton16(uint16_t x) { return flare::base::bit_swap16(x); }

FLARE_FORCE_INLINE uint32_t flare_hton32(uint32_t x) { return flare::base::bit_swap32(x); }

FLARE_FORCE_INLINE uint64_t flare_hton64(uint64_t x) { return flare::base::bit_swap64(x); }

#elif defined FLARE_SYSTEM_BIG_ENDIAN

// These definitions are simpler on big-endian machines
// These are functions instead of macros to avoid self-assignment warnings
// on calls such as "i = ghtnol(i);".  This also provides type checking.
FLARE_FORCE_INLINE uint16_t flare_hton16(uint16_t x) { return x; }
FLARE_FORCE_INLINE uint32_t flare_hton32(uint32_t x) { return x; }
FLARE_FORCE_INLINE uint64_t flare_hton64(uint64_t x) { return x; }

#else
#error \
    "Unsupported byte order: Either FLARE_SYSTEM_BIG_ENDIAN or " \
       "FLARE_SYSTEM_LITTLE_ENDIAN must be defined"
#endif  // byte order

FLARE_FORCE_INLINE uint16_t flare_ntoh16(uint16_t x) { return flare_hton16(x); }

FLARE_FORCE_INLINE uint32_t flare_ntoh32(uint32_t x) { return flare_hton32(x); }

FLARE_FORCE_INLINE uint64_t flare_ntoh64(uint64_t x) { return flare_hton64(x); }

// Utilities to convert numbers between the current hosts's native byte
// order and little-endian byte order
//
// Load/Store methods are alignment safe
namespace little_endian {
// Conversion functions.
#ifdef FLARE_SYSTEM_LITTLE_ENDIAN

FLARE_FORCE_INLINE uint16_t from_host16(uint16_t x) { return x; }

FLARE_FORCE_INLINE uint16_t to_host16(uint16_t x) { return x; }

FLARE_FORCE_INLINE uint32_t from_host32(uint32_t x) { return x; }

FLARE_FORCE_INLINE uint32_t to_host32(uint32_t x) { return x; }

FLARE_FORCE_INLINE uint64_t from_host64(uint64_t x) { return x; }

FLARE_FORCE_INLINE uint64_t to_host64(uint64_t x) { return x; }

FLARE_FORCE_INLINE constexpr bool is_little_endian() { return true; }

#elif defined FLARE_SYSTEM_BIG_ENDIAN

FLARE_FORCE_INLINE uint16_t from_host16(uint16_t x) { return flare::base::bit_swap16(x); }
FLARE_FORCE_INLINE uint16_t to_host16(uint16_t x) { return flare::base::bit_swap16(x); }

FLARE_FORCE_INLINE uint32_t from_host32(uint32_t x) { return flare::base::bit_swap32(x); }
FLARE_FORCE_INLINE uint32_t to_host32(uint32_t x) { return flare::base::bit_swap32(x); }

FLARE_FORCE_INLINE uint64_t from_host64(uint64_t x) { return flare::base::bit_swap64(x); }
FLARE_FORCE_INLINE uint64_t to_host64(uint64_t x) { return flare::base::bit_swap64(x); }

FLARE_FORCE_INLINE constexpr bool is_little_endian() { return false; }

#endif /* ENDIAN */

// Functions to do unaligned loads and stores in little-endian order.
FLARE_FORCE_INLINE uint16_t load16(const void *p) {
    return to_host16(FLARE_INTERNAL_UNALIGNED_LOAD16(p));
}

FLARE_FORCE_INLINE void store16(void *p, uint16_t v) {
    FLARE_INTERNAL_UNALIGNED_STORE16(p, from_host16(v));
}

FLARE_FORCE_INLINE uint32_t load32(const void *p) {
    return to_host32(FLARE_INTERNAL_UNALIGNED_LOAD32(p));
}

FLARE_FORCE_INLINE void store32(void *p, uint32_t v) {
    FLARE_INTERNAL_UNALIGNED_STORE32(p, from_host32(v));
}

FLARE_FORCE_INLINE uint64_t load64(const void *p) {
    return to_host64(FLARE_INTERNAL_UNALIGNED_LOAD64(p));
}

FLARE_FORCE_INLINE void store64(void *p, uint64_t v) {
    FLARE_INTERNAL_UNALIGNED_STORE64(p, from_host64(v));
}

}  // namespace little_endian

// Utilities to convert numbers between the current hosts's native byte
// order and big-endian byte order (same as network byte order)
//
// Load/Store methods are alignment safe
namespace big_endian {
#ifdef FLARE_SYSTEM_LITTLE_ENDIAN

FLARE_FORCE_INLINE uint16_t from_host16(uint16_t x) { return flare::base::bit_swap16(x); }

FLARE_FORCE_INLINE uint16_t to_host16(uint16_t x) { return flare::base::bit_swap16(x); }

FLARE_FORCE_INLINE uint32_t from_host32(uint32_t x) { return flare::base::bit_swap32(x); }

FLARE_FORCE_INLINE uint32_t to_host32(uint32_t x) { return flare::base::bit_swap32(x); }

FLARE_FORCE_INLINE uint64_t from_host64(uint64_t x) { return flare::base::bit_swap64(x); }

FLARE_FORCE_INLINE uint64_t to_host64(uint64_t x) { return flare::base::bit_swap64(x); }

FLARE_FORCE_INLINE constexpr bool is_little_endian() { return true; }

#elif defined FLARE_SYSTEM_BIG_ENDIAN

FLARE_FORCE_INLINE uint16_t from_host16(uint16_t x) { return x; }
FLARE_FORCE_INLINE uint16_t to_host16(uint16_t x) { return x; }

FLARE_FORCE_INLINE uint32_t from_host32(uint32_t x) { return x; }
FLARE_FORCE_INLINE uint32_t to_host32(uint32_t x) { return x; }

FLARE_FORCE_INLINE uint64_t from_host64(uint64_t x) { return x; }
FLARE_FORCE_INLINE uint64_t to_host1664(uint64_t x) { return x; }

FLARE_FORCE_INLINE constexpr bool is_little_endian() { return false; }

#endif /* ENDIAN */

// Functions to do unaligned loads and stores in big-endian order.
FLARE_FORCE_INLINE uint16_t load16(const void *p) {
    return to_host16(FLARE_INTERNAL_UNALIGNED_LOAD16(p));
}

FLARE_FORCE_INLINE void store16(void *p, uint16_t v) {
    FLARE_INTERNAL_UNALIGNED_STORE16(p, from_host16(v));
}

FLARE_FORCE_INLINE uint32_t load32(const void *p) {
    return to_host32(FLARE_INTERNAL_UNALIGNED_LOAD32(p));
}

FLARE_FORCE_INLINE void store32(void *p, uint32_t v) {
    FLARE_INTERNAL_UNALIGNED_STORE32(p, from_host32(v));
}

FLARE_FORCE_INLINE uint64_t load64(const void *p) {
    return to_host64(FLARE_INTERNAL_UNALIGNED_LOAD64(p));
}

FLARE_FORCE_INLINE void store64(void *p, uint64_t v) {
    FLARE_INTERNAL_UNALIGNED_STORE64(p, from_host64(v));
}

}  // namespace big_endian

}  // namespace flare::base

#endif  // FLARE_SYSTEM_ENDIAN_H_
