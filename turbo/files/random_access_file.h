
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef TURBO_FILES_RANDOM_ACCESS_FILE_H_
#define TURBO_FILES_RANDOM_ACCESS_FILE_H_

#include "turbo/files/filesystem.h"
#include "turbo/base/profile.h"
#include "turbo/base/result_status.h"
#include "turbo/io/cord_buf.h"
#include <fcntl.h>

namespace turbo {

    class random_access_file {
    public:
        random_access_file() noexcept = default;

        ~random_access_file();

        result_status open(const turbo::file_path &path) noexcept;

        result_status read(size_t n, off_t offset, std::string *content);

        result_status read(size_t n, off_t offset, turbo::cord_buf *buf);

        result_status read(size_t n, off_t offset, char *buf);

        bool is_eof(off_t off, size_t has_read, result_status *frs);

        void close();

        const turbo::file_path &path() const {
            return _path;
        }

    private:
        TURBO_DISALLOW_COPY_AND_ASSIGN(random_access_file);
        turbo::file_path _path;
        int _fd{-1};
    };

}  // namespace turbo

#endif // TURBO_FILES_RANDOM_ACCESS_FILE_H_
