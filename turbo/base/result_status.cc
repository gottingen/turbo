
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "turbo/base/result_status.h"
#include <cstring>
#include "turbo/base/errno.h"

namespace turbo {
    result_status result_status::from_turbo_error(int err) {
        const char * msg = turbo_error(err);
        return result_status(err, msg);
    }

    result_status result_status::from_turbo_error(int err, const std::string_view &ext) {
        const char * msg = turbo_error(err);
        return result_status(err, "{} {}", msg, ext);
    }

    result_status result_status::from_error_code(const std::error_code &ec) {
        return result_status(ec.value(), ec.message());
    }

    result_status result_status::from_last_error() {
        std::error_code error;
        error.assign(errno, std::system_category());
        return from_error_code(error);
    }

}  // namespace turbo
