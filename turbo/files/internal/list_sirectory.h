
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_FILES_INTERNAL_LIST_SIRECTORY_H_
#define TURBO_FILES_INTERNAL_LIST_SIRECTORY_H_

#include <vector>
#include <string_view>
#include "turbo/files/filesystem.h"

namespace turbo::files_internal {
    std::vector<file_path> list_directory_internal(std::string_view path, bool recursive, std::error_code &ec);
}
#endif  // TURBO_FILES_INTERNAL_LIST_SIRECTORY_H_
