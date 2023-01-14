
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef TURBO_FILES_SEQUENTIAL_READ_FILE_H_
#define TURBO_FILES_SEQUENTIAL_READ_FILE_H_

#include "turbo/files/filesystem.h"
#include "turbo/base/profile.h"
#include "turbo/base/result_status.h"
#include "turbo/io/cord_buf.h"

namespace turbo {

    class sequential_read_file {
    public:
        sequential_read_file() noexcept = default;

        ~sequential_read_file();

        result_status open(const turbo::file_path &path) noexcept;

        result_status read(std::string *content, size_t n = npos);

        result_status read(turbo::cord_buf *buf, size_t n = npos);

        std::pair<result_status, size_t> read(void *buf, size_t n);

        result_status skip(size_t n);

        bool is_eof(result_status *frs);

        void close();

        void reset();

        size_t has_read() const {
            return _has_read;
        }

        const turbo::file_path &path() const {
            return _path;
        }

    private:
        TURBO_DISALLOW_COPY_AND_ASSIGN(sequential_read_file);

        static const size_t npos = std::numeric_limits<size_t>::max();
        int _fd{-1};
        turbo::file_path _path;
        size_t _has_read{0};
    };

}  // namespace turbo

#endif  // TURBO_FILES_SEQUENTIAL_READ_FILE_H_
