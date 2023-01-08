
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/fiber/internal/fiber.h"

namespace flare {

    int fiber_getconcurrency(void) {
        return ::fiber_getconcurrency();
    }

    int fiber_setconcurrency(int num) {
        return ::fiber_setconcurrency(num);
    }

}  // namespace flare
