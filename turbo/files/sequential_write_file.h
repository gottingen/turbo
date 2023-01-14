
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef TURBO_FILES_SEQUENTIAL_WRITE_FILE_H_
#define TURBO_FILES_SEQUENTIAL_WRITE_FILE_H_

#include "turbo/files/filesystem.h"
#include "turbo/base/profile.h"
#include "turbo/base/result_status.h"
#include "turbo/io/cord_buf.h"

namespace turbo {

    class sequential_write_file {
    public:
        sequential_write_file() = default;

        ~sequential_write_file();

        result_status open(const turbo::file_path &path, bool truncate = true) noexcept;

        result_status write(std::string_view content);

        result_status write(void *content, size_t n) {
            return write(std::string_view((char*)content, n));
        }

        result_status write(const turbo::cord_buf &data);

        void flush();

        void close();

        void reset(size_t n = 0);

        size_t has_write() const {
            return _has_write;
        }

        const turbo::file_path &path() const {
            return _path;
        }

    private:
        int _fd{-1};
        turbo::file_path _path;
        size_t _has_write;
    };
}  // namespace turbo
#endif  // TURBO_FILES_SEQUENTIAL_WRITE_FILE_H_
