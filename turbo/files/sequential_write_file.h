// Copyright 2023 The Turbo Authors.
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

#ifndef TURBO_FILES_SEQUENTIAL_WRITE_FILE_H_
#define TURBO_FILES_SEQUENTIAL_WRITE_FILE_H_

#include "turbo/base/result_status.h"
#include "turbo/files/filesystem.h"
#include "turbo/strings/cord.h"
#include "turbo/files/file_event_listener.h"
#include "turbo/files/file_option.h"
#include "turbo/format/format.h"

namespace turbo {

    class SequentialWriteFile {
    public:
        SequentialWriteFile() = default;

        explicit SequentialWriteFile(const FileEventListener &listener);

        ~SequentialWriteFile();

        void set_option(const FileOption &option);

        turbo::Status open(const turbo::filesystem::path &fname,
                           bool truncate = false);

        turbo::Status reopen(bool truncate = false);

        turbo::Status write(const char *data, size_t size);

        turbo::Status write(std::string_view str) {
            return write(str.data(), str.size());
        }

        template<typename Char, size_t N>
        turbo::Status write(const fmt::basic_memory_buffer<Char, N> &buffer) {
            return write(buffer.data(), buffer.size() * sizeof(Char));
        }

        turbo::ResultStatus<size_t> size() const;

        void close();

        turbo::Status flush();

        const turbo::filesystem::path &file_path() const;

    private:
        static const size_t npos = std::numeric_limits<size_t>::max();
        std::FILE *_fd{nullptr};
        turbo::filesystem::path _file_path;
        turbo::FileOption _option;
        FileEventListener _listener;
    };
} // namespace turbo

#endif // TURBO_FILES_SEQUENTIAL_WRITE_FILE_H_
