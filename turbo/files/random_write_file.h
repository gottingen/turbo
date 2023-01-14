
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_FILES_RANDOM_WRITE_FILE_H_
#define TURBO_FILES_RANDOM_WRITE_FILE_H_

#include "turbo/files/filesystem.h"
#include "turbo/base/profile.h"
#include "turbo/base/result_status.h"
#include "turbo/io/cord_buf.h"
#include <fcntl.h>

namespace turbo {

    class random_write_file {
    public:
        random_write_file() = default;

        ~random_write_file();

        result_status open(const turbo::file_path &path, bool truncate = true) noexcept;

        result_status write(off_t offset, std::string_view content);

        result_status write(off_t offset, const void *content, size_t size);

        result_status write(off_t offset, const turbo::cord_buf &data);

        void close();

        void flush();

        const turbo::file_path &path() const {
            return _path;
        }

    private:
        int _fd{-1};
        turbo::file_path _path;
    };

}  // namespace turbo

#endif  // TURBO_FILES_RANDOM_WRITE_FILE_H_
