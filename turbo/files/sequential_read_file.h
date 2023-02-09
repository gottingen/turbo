// Copyright 2022 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
