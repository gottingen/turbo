
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#ifndef TURBO_FILES_SEQUENTIAL_READ_FILE_H_
#define TURBO_FILES_SEQUENTIAL_READ_FILE_H_

#include "turbo/base/status.h"
#include "turbo/files/filesystem.h"
#include "turbo/platform/port.h"
#include "turbo/strings/cord.h"

namespace turbo {

    class SequentialReadFile {
    public:
        SequentialReadFile() noexcept = default;

        ~SequentialReadFile();

        turbo::Status open(const turbo::filesystem::path &path) noexcept;

        turbo::Status read(std::string *content, size_t n = npos);

        turbo::Status read(turbo::Cord *buf, size_t n = npos);

        turbo::Status skip(off_t n);

        bool is_eof(turbo::Status *frs);

        void close();

        void reset();

        size_t has_read() const {
            return _has_read;
        }

        const turbo::filesystem::path &path() const {
            return _path;
        }

    private:
        TURBO_NON_COPYABLE(SequentialReadFile);

        static const size_t npos = std::numeric_limits<size_t>::max();
        int _fd{-1};
        turbo::filesystem::path _path;
        size_t _has_read{0};
    };

}  // namespace turbo

#endif  // TURBO_FILES_SEQUENTIAL_READ_FILE_H_
