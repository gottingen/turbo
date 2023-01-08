
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#ifndef FLARE_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_
#define FLARE_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_

#include "flare/base/profile.h"

namespace flare::debugging {

    namespace debugging_internal {

        // Return whether the byte at *addr is readable, without faulting.
        // Save and restores errno.
        bool address_is_readable(const void *addr);

    }  // namespace debugging_internal

}  // namespace flare::debugging

#endif  // FLARE_DEBUGGING_INTERNAL_ADDRESS_IS_READABLE_H_
