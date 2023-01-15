
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_STRINGS_UTF8_NAIVE_DECODER_H_
#define TURBO_STRINGS_UTF8_NAIVE_DECODER_H_

#include <cstddef>
#include <cstdint>

namespace turbo::utf8_detail {

    ptrdiff_t naive_decoder(unsigned char const *s_ptr,
                           unsigned char const *s_ptr_end,
                           char32_t *dest) noexcept;

}  // namespace turbo::utf8_detail
#endif  // TURBO_STRINGS_UTF8_NAIVE_DECODER_H_
