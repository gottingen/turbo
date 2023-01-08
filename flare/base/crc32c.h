
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_CRC32C_H_
#define FLARE_BASE_CRC32C_H_

#include <stddef.h>
#include <stdint.h>

namespace flare::base {

    extern bool is_fast_crc32_supported();

    // Return the crc32c of concat(A, data[0,n-1]) where init_crc is the
    // crc32c of some string A.  extend() is often used to maintain the
    // crc32c of a stream of data.
    extern uint32_t extend(uint32_t init_crc, const char *data, size_t n);

    // Return the crc32c of data[0,n-1]
    inline uint32_t value(const char *data, size_t n) {
        return extend(0, data, n);
    }

    static const uint32_t kMaskDelta = 0xa282ead8ul;

    // Return a masked representation of crc.
    //
    // Motivation: it is problematic to compute the CRC of a string that
    // contains embedded CRCs.  Therefore we recommend that CRCs stored
    // somewhere (e.g., in files) should be masked before being stored.
    inline uint32_t mask(uint32_t crc) {
        // Rotate right by 15 bits and add a constant.
        return ((crc >> 15) | (crc << 17)) + kMaskDelta;
    }

    // Return the crc whose masked representation is masked_crc.
    inline uint32_t unmask(uint32_t masked_crc) {
        uint32_t rot = masked_crc - kMaskDelta;
        return ((rot >> 17) | (rot << 15));
    }

}  // namespace flare::base

#endif  // FLARE_BASE_CRC32C_H_
