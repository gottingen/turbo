//
// Created by liyinbin on 2022/9/5.
//

#ifndef FLARE_FILES_CONSTANTS_H_
#define FLARE_FILES_CONSTANTS_H_


/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <cstdint>
#include "flare/base/profile.h"

namespace flare {

    using file_handle_type = int;

    // This value represents an invalid file handle type. This can be used to
    // determine whether `basic_mmap::file_handle` is valid, for example.
    const static file_handle_type invalid_handle = -1;

    inline constexpr bool is_valid_handle(const file_handle_type &handle) {
        return handle == invalid_handle;
    }

}  // namespace flare

#endif  // FLARE_FILES_CONSTANTS_H_
