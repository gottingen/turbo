
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_FILES_RANDOM_WRITE_FILE_H_
#define FLARE_FILES_RANDOM_WRITE_FILE_H_

#include "flare/files/filesystem.h"
#include "flare/base/profile.h"
#include "flare/base/result_status.h"
#include "flare/io/cord_buf.h"
#include <fcntl.h>

namespace flare {

    class random_write_file {
    public:
        random_write_file() = default;

        ~random_write_file();

        result_status open(const flare::file_path &path, bool truncate = true) noexcept;

        result_status write(off_t offset, std::string_view content);

        result_status write(off_t offset, const void *content, size_t size);

        result_status write(off_t offset, const flare::cord_buf &data);

        void close();

        void flush();

        const flare::file_path &path() const {
            return _path;
        }

    private:
        int _fd{-1};
        flare::file_path _path;
    };

}  // namespace flare

#endif  // FLARE_FILES_RANDOM_WRITE_FILE_H_
