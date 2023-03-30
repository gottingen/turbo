// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SIMDUTF_ASCII_H
#define SIMDUTF_ASCII_H

namespace simdutf {
namespace scalar {
namespace {
namespace ascii {
#if SIMDUTF_IMPLEMENTATION_FALLBACK
// Only used by the fallback kernel.
inline simdutf_warn_unused bool validate(const char *buf, size_t len) noexcept {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
    uint64_t pos = 0;
    // process in blocks of 16 bytes when possible
    for (;pos + 16 < len; pos += 16) {
        uint64_t v1;
        std::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2};
        if ((v & 0x8080808080808080) != 0) { return false; }
    }
    // process the tail byte-by-byte
    for (;pos < len; pos ++) {
        if (data[pos] >= 0b10000000) { return false; }
    }
    return true;
}
#endif

inline simdutf_warn_unused result validate_with_errors(const char *buf, size_t len) noexcept {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
    size_t pos = 0;
    // process in blocks of 16 bytes when possible
    for (;pos + 16 < len; pos += 16) {
        uint64_t v1;
        std::memcpy(&v1, data + pos, sizeof(uint64_t));
        uint64_t v2;
        std::memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
        uint64_t v{v1 | v2};
        if ((v & 0x8080808080808080) != 0) {
            for (;pos < len; pos ++) {
                if (data[pos] >= 0b10000000) { return result(error_code::TOO_LARGE, pos); }
            }
        }
    }
    // process the tail byte-by-byte
    for (;pos < len; pos ++) {
        if (data[pos] >= 0b10000000) { return result(error_code::TOO_LARGE, pos); }
    }
    return result(error_code::SUCCESS, pos);
}

} // ascii namespace
} // unnamed namespace
} // namespace scalar
} // namespace simdutf

#endif