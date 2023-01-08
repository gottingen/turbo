
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/files/internal/list_sirectory.h"

namespace flare::files_internal {

    std::vector<file_path> list_directory_internal(std::string_view path, bool recursive, std::error_code &ec) {
        std::vector<file_path> files;
        if (recursive) {
            recursive_directory_iterator dit(path, ec);
            if (ec) {
                return files;
            }
            for (auto &di : dit) {
                if (di.file_path() != "." && di.file_path() != "..") {
                    files.push_back(di.file_path());
                }
            }
        } else {
            directory_iterator dit(path, ec);
            if (ec) {
                return files;
            }
            for (auto &di : dit) {
                if (di.file_path() != "." && di.file_path() != "..") {
                    files.push_back(di.file_path());
                }
            }
        }
        return files;
    }

}  // namespace flare::files_internal
