
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_STRINGS_UTF8_NAIVE_ENCODER_H_
#define TURBO_STRINGS_UTF8_NAIVE_ENCODER_H_

#include <cstddef>
#include <cstdint>

namespace turbo::utf8_detail {

    size_t naive_count_bytes_size(const uint32_t* s_ptr, const uint32_t* s_ptr_end) noexcept;

    ptrdiff_t naive_encoder(const uint32_t* s_ptr,
                           const uint32_t* s_ptr_end,
                           unsigned char* dst) noexcept;
}  // namespace turbo::utf8_detail

#endif  // TURBO_STRINGS_UTF8_NAIVE_ENCODER_H_
