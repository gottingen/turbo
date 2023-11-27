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

#ifndef TURBO_FILES_RANDOM_READ_FILE_H_
#define TURBO_FILES_RANDOM_READ_FILE_H_

#include "turbo/base/result_status.h"
#include "turbo/files/filesystem.h"
#include "turbo/platform/port.h"
#include "turbo/strings/cord.h"
#include "turbo/files/file_event_listener.h"
#include "turbo/files/file_option.h"
#include "turbo/files/fio.h"

namespace turbo {

    class RandomReadFile {
    public:
        RandomReadFile() = default;

        explicit RandomReadFile(const FileEventListener &listener);

        ~RandomReadFile();

        ///
        /// \param path
        /// \return
        [[nodiscard]] turbo::Status open(const turbo::filesystem::path &path) noexcept;

        ///
        /// \param offset
        /// \param content
        /// \param n
        /// \return
        [[nodiscard]] turbo::ResultStatus<size_t> read(size_t offset, std::string *content, size_t n = npos);

        ///
        /// \param offset
        /// \param buf
        /// \param n
        /// \return
        [[nodiscard]] turbo::ResultStatus<size_t> read(size_t offset, turbo::Cord *buf, size_t n = npos);

        ///
        /// \param offset
        /// \param buff
        /// \param len
        /// \return
        [[nodiscard]] turbo::ResultStatus<size_t> read(size_t offset, void *buff, size_t len);

        ///
        /// \brief close file
        void close();

        ///
        /// \return
        const turbo::filesystem::path &path() const { return _file_path; }

    private:
        // no lint
        TURBO_NON_COPYABLE(RandomReadFile);

        static const size_t npos = std::numeric_limits<size_t>::max();
        std::FILE *_fp{nullptr};
        int        _fd;
        turbo::filesystem::path _file_path;
        turbo::FileOption _option;
        FileEventListener _listener;
    };

} // namespace turbo

#endif  // TURBO_FILES_RANDOM_READ_FILE_H_
