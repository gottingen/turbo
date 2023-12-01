// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//
//
// Created by jeff on 23-11-28.
//

#ifndef TURBO_FILES_RANDOM_WRITE_FILE_H_
#define TURBO_FILES_RANDOM_WRITE_FILE_H_

#include "turbo/base/result_status.h"
#include "turbo/files/filesystem.h"
#include "turbo/platform/port.h"
#include "turbo/strings/cord.h"
#include "turbo/files/file_event_listener.h"
#include "turbo/files/file_option.h"
#include "turbo/files/fio.h"

namespace turbo {

    class RandomWriteFile {
    public:

        RandomWriteFile() = default;

        explicit RandomWriteFile(const FileEventListener &listener);

        ///

        ~RandomWriteFile();

        ///
        /// \param option
        void set_option(const FileOption &option);

        ///
        /// \param fname
        /// \param truncate
        /// \return
        [[nodiscard]] turbo::Status open(const turbo::filesystem::path &fname,
                           bool truncate = false);

        ///
        /// \param truncate
        /// \return
        [[nodiscard]] turbo::Status reopen(bool truncate = false);

        ///
        /// \param offset
        /// \param data
        /// \param size
        /// \param truncate
        /// \return
        [[nodiscard]] turbo::Status write(size_t offset,const char *data, size_t size, bool truncate = false);

        ///
        /// \param offset
        /// \param str
        /// \param truncate
        /// \return
        [[nodiscard]] turbo::Status write(size_t offset, std::string_view str, bool truncate = false) {
            return write(offset, str.data(), str.size(), truncate);
        }

        ///
        /// \param size
        /// \return
        [[nodiscard]] turbo::Status truncate(size_t size);

        ///
        /// \return
        [[nodiscard]] turbo::ResultStatus<size_t> size() const;

        ///

        void close();

        ///
        /// \return
        turbo::Status flush();

        ///
        /// \return
        [[nodiscard]] const turbo::filesystem::path &file_path() const;

    private:
        static const size_t npos = std::numeric_limits<size_t>::max();
        std::FILE *_fp{nullptr};
        int        _fd{-1};
        turbo::filesystem::path _file_path;
        turbo::FileOption _option;
        FileEventListener _listener;
    };
}  // namespace turbo

#endif  // TURBO_FILES_RANDOM_WRITE_FILE_H_
