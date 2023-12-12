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

#include "turbo/base/result_status.h"
#include "turbo/files/filesystem.h"
#include "turbo/platform/port.h"
#include "turbo/strings/cord.h"
#include "turbo/files/file_event_listener.h"
#include "turbo/files/file_option.h"

namespace turbo {

    /**
     * @ingroup turbo_files_read_file
     * @brief SequentialReadFile is a file io utility class.
     * eg.
     * @code
     *     SequentialReadFile file;
     *     auto rs = file.open("test.txt");
     *     if (!rs.ok()) {
     *          std::cout << rs.status().message() << std::endl;
     *          return;
     *     }
     *     std::string content;
     *     // read all content.
     *     rs = file.read(&content);
     *     if (!rs.ok()) {
     *         std::cout << rs.status().message() << std::endl;
     *         return;
     *     }
     *     std::cout << content << std::endl;
     *     // read 10 bytes.
     *     rs = file.read(&content, 10);
     *     if (!rs.ok()) {
     *         std::cout << rs.status().message() << std::endl;
     *         return;
     *     }
     *     std::cout << content << std::endl;
     *     // close file or use RAII.
     *     file.close();
     * @endcode
     */
    class SequentialReadFile {
    public:
        SequentialReadFile() = default;

        explicit SequentialReadFile(const FileEventListener &listener);

        ~SequentialReadFile();

        /**
         * @brief open file with path and option specified by user.
         *        If the file does not exist, it will be returned with an error.
         * @param path file path
         * @param option file option
         */
        [[nodiscard]] turbo::Status open(const turbo::filesystem::path &path, const turbo::FileOption &option = FileOption::kDefault) noexcept;

        /**
         * @brief read file content sequentially from the current position to the specified length.
         * @param content [output] file content, can not be nullptr.
         * @param n [input] read length, default is npos, which means read all. If the length is greater than the file
         *          size, the file content will be read from the current position to the end of the file.
         * @return the length of the file content read and the status of the operation.
         */
        [[nodiscard]] turbo::ResultStatus<size_t> read(std::string *content, size_t n = npos);

        /**
         * @brief read file content sequentially from the current position to the specified length.
         * @param buf [output] file content, can not be nullptr.
         * @param n [input] read length, default is npos, which means read all. If the length is greater than the file
         *          size, the file content will be read from the current position to the end of the file.
         * @return the length of the file content read and the status of the operation.
         */
        [[nodiscard]] turbo::ResultStatus<size_t> read(turbo::Cord *buf, size_t n = npos);


        /**
         * @brief read file content sequentially from the current position to the specified length.
         * @param buff [output] file content, can not be nullptr.
         * @param len [input] read length, The length must be less than or equal to the size of the buff.
         *        If the length is greater than the file size, the file content will be read from the current position to the end of the file.
         *        If the length is less than the file size, the file content will be read from the current position to the length.
         * @return the length of the file content read and the status of the operation.
         */
        [[nodiscard]] turbo::ResultStatus<size_t> read(void *buff, size_t len);

        /**
         * @brief skip file descriptor sequentially from the current position to the position specified by offset.
         *        after skip, the current position will be offset + current position.
         * @param n [input] skip length, if n + current position is greater than the file size, the current position will be set to the end of the file.
         * @return the status of the operation.
         */
        [[nodiscard]] turbo::Status skip(off_t n);

        /**
         * @brief if the current position is the end of the file, return true, otherwise return false.
         * @return the status of the operation.
         */
        turbo::ResultStatus<bool> is_eof();

        /**
         * @brief close file.
         */
        void close();

        /**
         * @brief get file path.
         * @return file path.
         */
        [[nodiscard]] const turbo::filesystem::path &path() const { return _file_path; }

    private:
        // no lint
        TURBO_NON_COPYABLE(SequentialReadFile);

        static const size_t npos = std::numeric_limits<size_t>::max();
        std::FILE *_fp{nullptr};
        turbo::filesystem::path _file_path;
        turbo::FileOption _option;
        FileEventListener _listener;
    };

} // namespace turbo

#endif  // TURBO_FILES_SEQUENTIAL_READ_FILE_H_
