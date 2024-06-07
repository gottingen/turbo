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
// -----------------------------------------------------------------------------
// File: crc32c.h
// -----------------------------------------------------------------------------
//
// This header file defines the API for computing CRC32C values as checksums
// for arbitrary sequences of bytes provided as a string buffer.
//
// The API includes the basic functions for computing such CRC32C values and
// some utility functions for performing more efficient mathematical
// computations using an existing checksum.
#pragma once

#include <cstdint>
#include <ostream>

#include <turbo/crypto/internal/crc32c_inline.h>
#include <turbo/strings/str_format.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    //-----------------------------------------------------------------------------
    // CRC32C
    //-----------------------------------------------------------------------------

    // `CRC32C` defines a strongly-typed integer for holding a CRC32C value.
    //
    // Some operators are intentionally omitted. Only equality operators are defined
    // so that `CRC32C` can be directly compared. Methods for putting `CRC32C`
    // directly into a set are omitted because this is bug-prone due to checksum
    // collisions. Use an explicit conversion to the `uint32_t` space for operations
    // that treat `CRC32C` as an integer.
    class CRC32C final {
    public:
        CRC32C() = default;

        constexpr explicit CRC32C(uint32_t crc) : crc_(crc) {}

        CRC32C(const CRC32C &) = default;

        CRC32C &operator=(const CRC32C &) = default;

        explicit operator uint32_t() const { return crc_; }

        friend bool operator==(CRC32C lhs, CRC32C rhs) {
            return static_cast<uint32_t>(lhs) == static_cast<uint32_t>(rhs);
        }

        friend bool operator!=(CRC32C lhs, CRC32C rhs) { return !(lhs == rhs); }

        template<typename Sink>
        friend void turbo_stringify(Sink &sink, CRC32C crc) {
            turbo::format(&sink, "%08x", static_cast<uint32_t>(crc));
        }

    private:
        uint32_t crc_;
    };


    namespace crc_internal {
        // Non-inline code path for `turbo::extend_crc32c()`. Do not call directly.
        // Call `turbo::extend_crc32c()` (defined below) instead.
        CRC32C extend_crc32c_internal(CRC32C initial_crc,
                                      std::string_view buf_to_add);
    }  // namespace crc_internal

    // -----------------------------------------------------------------------------
    // CRC32C Computation Functions
    // -----------------------------------------------------------------------------

    // compute_crc32c()
    //
    // Returns the CRC32C value of the provided string.
    CRC32C compute_crc32c(std::string_view buf);

    // extend_crc32c()
    //
    // Computes a CRC32C value from an `initial_crc` CRC32C value including the
    // `buf_to_add` bytes of an additional buffer. Using this function is more
    // efficient than computing a CRC32C value for the combined buffer from
    // scratch.
    //
    // Note: `extend_crc32c` with an initial_crc of 0 is equivalent to
    // `compute_crc32c`.
    //
    // This operation has a runtime cost of O(`buf_to_add.size()`)
    inline CRC32C extend_crc32c(CRC32C initial_crc,
                                 std::string_view buf_to_add) {
        // Approximately 75% of calls have size <= 64.
        if (buf_to_add.size() <= 64) {
            uint32_t crc = static_cast<uint32_t>(initial_crc);
            if (crc_internal::ExtendCrc32cInline(&crc, buf_to_add.data(),
                                                 buf_to_add.size())) {
                return CRC32C{crc};
            }
        }
        return crc_internal::extend_crc32c_internal(initial_crc, buf_to_add);
    }

    // extend_crc32c_by_zeroes()
    //
    // Computes a CRC32C value for a buffer with an `initial_crc` CRC32C value,
    // where `length` bytes with a value of 0 are appended to the buffer. Using this
    // function is more efficient than computing a CRC32C value for the combined
    // buffer from scratch.
    //
    // This operation has a runtime cost of O(log(`length`))
    CRC32C extend_crc32c_by_zeroes(CRC32C initial_crc, size_t length);

    // memcpy_crc32c()
    //
    // Copies `src` to `dest` using `memcpy()` semantics, returning the CRC32C
    // value of the copied buffer.
    //
    // Using `memcpy_crc32c()` is potentially faster than performing the `memcpy()`
    // and `compute_crc32c()` operations separately.
    CRC32C memcpy_crc32c(void *dest, const void *src, size_t count,
                           CRC32C initial_crc = CRC32C{0});

    // -----------------------------------------------------------------------------
    // CRC32C Arithmetic Functions
    // -----------------------------------------------------------------------------

    // The following functions perform arithmetic on CRC32C values, which are
    // generally more efficient than recalculating any given result's CRC32C value.

    // concat_crc32c()
    //
    // Calculates the CRC32C value of two buffers with known CRC32C values
    // concatenated together.
    //
    // Given a buffer with CRC32C value `crc1` and a buffer with
    // CRC32C value `crc2` and length, `crc2_length`, returns the CRC32C value of
    // the concatenation of these two buffers.
    //
    // This operation has a runtime cost of O(log(`crc2_length`)).
    CRC32C concat_crc32c(CRC32C crc1, CRC32C crc2, size_t crc2_length);

    // remove_crc32c_prefix()
    //
    // Calculates the CRC32C value of an existing buffer with a series of bytes
    // (the prefix) removed from the beginning of that buffer.
    //
    // Given the CRC32C value of an existing buffer, `full_string_crc`; The CRC32C
    // value of a prefix of that buffer, `prefix_crc`; and the length of the buffer
    // with the prefix removed, `remaining_string_length` , return the CRC32C
    // value of the buffer with the prefix removed.
    //
    // This operation has a runtime cost of O(log(`remaining_string_length`)).
    CRC32C remove_crc32c_prefix(CRC32C prefix_crc, CRC32C full_string_crc,
                                size_t remaining_string_length);

    // remove_crc32c_suffix()
    //
    // Calculates the CRC32C value of an existing buffer with a series of bytes
    // (the suffix) removed from the end of that buffer.
    //
    // Given a CRC32C value of an existing buffer `full_string_crc`, the CRC32C
    // value of the suffix to remove `suffix_crc`, and the length of that suffix
    // `suffix_len`, returns the CRC32C value of the buffer with suffix removed.
    //
    // This operation has a runtime cost of O(log(`suffix_len`))
    CRC32C remove_crc32c_suffix(CRC32C full_string_crc, CRC32C suffix_crc,
                                size_t suffix_length);

    // operator<<
    //
    // Streams the CRC32C value `crc` to the stream `os`.
    inline std::ostream &operator<<(std::ostream &os, CRC32C crc) {
        return os << turbo::stream_format("%08x", static_cast<uint32_t>(crc));
    }

}  // namespace turbo
