
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef FLARE_FILES_SEQUENTIAL_READ_FILE_H_
#define FLARE_FILES_SEQUENTIAL_READ_FILE_H_

#include "flare/files/filesystem.h"
#include "flare/base/profile.h"
#include "flare/base/result_status.h"
#include "flare/io/cord_buf.h"

namespace flare {

    class sequential_read_file {
    public:
        sequential_read_file() noexcept = default;

        ~sequential_read_file();

        result_status open(const flare::file_path &path) noexcept;

        result_status read(std::string *content, size_t n = npos);

        result_status read(flare::cord_buf *buf, size_t n = npos);

        std::pair<result_status, size_t> read(void *buf, size_t n);

        result_status skip(size_t n);

        bool is_eof(result_status *frs);

        void close();

        void reset();

        size_t has_read() const {
            return _has_read;
        }

        const flare::file_path &path() const {
            return _path;
        }

    private:
        FLARE_DISALLOW_COPY_AND_ASSIGN(sequential_read_file);

        static const size_t npos = std::numeric_limits<size_t>::max();
        int _fd{-1};
        flare::file_path _path;
        size_t _has_read{0};
    };

}  // namespace flare

#endif  // FLARE_FILES_SEQUENTIAL_READ_FILE_H_
