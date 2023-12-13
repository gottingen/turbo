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

    /**
     * @ingroup turbo_files_write_file
     * @brief SequentialWriteFile is a file io utility class.
     * eg.
     * @code
     *     SequentialWriteFile file;
     *     auto rs = file.open("test.txt");
     *     if (!rs.ok()) {
     *         std::cout << rs.status().message() << std::endl;
     *         return;
     *     }
     *     std::string content = "hello world";
     *     // write content to file.
     *     rs = file.write(content);
     *     if (!rs.ok()) {
     *         std::cout << rs.status().message() << std::endl;
     *         return;
     *     }
     *     // flush file.
     *     rs = file.flush();
     *     if (!rs.ok()) {
     *         std::cout << rs.status().message() << std::endl;
     *         return;
     *      }
     *      // close file or use RAII, it is not necessary to call flush before close.
     *      // and recommend to use close manually.
     *      file.close();
     *      // or use RAII.
     * @endcode
     */
    class SequentialWriteFile {
    public:
        SequentialWriteFile() = default;

        /**
         * @brief set_option set file option before open file.
         *        default option is FileOption::kDefault.
         *        If you want to set the file option, you must call this function before open file.
         */
        explicit SequentialWriteFile(const FileEventListener &listener);

        ~SequentialWriteFile();

        /**
         * @brief set_option set file option before open file.
         *        default option is FileOption::kDefault.
         *        If you want to set the file option, you must call this function before open file.
         */
        void set_option(const FileOption &option);

        /**
         * @brief open file with path and option specified by user.
         *        The option can be set by set_option function. @see set_option.
         *        If the file does not exist, it will be created.
         *        If the file exists, it will be opened.
         *        If the file exists and the truncate option is true, the file will be truncated.
         * @param fname file path
         * @param truncate if true, the file will be truncated.
         * @return the status of the operation.
         */

        [[nodiscard]] turbo::Status open(const turbo::filesystem::path &fname,
                           bool truncate = false);

        /**
         * @brief reopen file with path and option specified by user.
         *        The option can be set by set_option function. @see set_option.
         *        If the file does not exist, it will be created.
         *        If the file exists, it will be opened.
         *        If the file exists and the truncate option is true, the file will be truncated.
         * @param fname file path
         * @param truncate if true, the file will be truncated.
         * @return the status of the operation.
         */
        [[nodiscard]] turbo::Status reopen(bool truncate = false);

        /**
         * @brief write file content to the end of the file.
         * @param data [input] file content, can not be nullptr.
         * @param size [input] write length.
         * @return the status of the operation.
         */
        [[nodiscard]] turbo::Status write(const char *data, size_t size);

        /**
         * @brief write file content to the end of the file.
         * @param str [input] file content, can not be empty.
         * @return the status of the operation.
         */
        [[nodiscard]] turbo::Status write(std::string_view str) {
            return write(str.data(), str.size());
        }

        /**
         * @brief write file content to the end of the file.
         * @param buffer [input] file content, can not be empty.
         * @return the status of the operation.
         */
        template<typename Char, size_t N>
        [[nodiscard]] turbo::Status write(const turbo::basic_memory_buffer<Char, N> &buffer) {
            return write(buffer.data(), buffer.size() * sizeof(Char));
        }

        /**
         * @brief truncate file to the specified length.
         * @param size [input] file length.
         * @return the status of the operation.
         */
        [[nodiscard]] turbo::Status truncate(size_t size);

        /**
         * @brief get file size.
         * @return the file size and the status of the operation.
         */
        [[nodiscard]] turbo::ResultStatus<size_t> size() const;

        /**
         * @brief close file.
         */
        void close();

        /**
         * @brief flush file.
         */
        [[nodiscard]]
        turbo::Status flush();

        /**
         * @brief get file path.
         * @return file path.
         */
        [[nodiscard]] const turbo::filesystem::path &file_path() const;

    private:
        static const size_t npos = std::numeric_limits<size_t>::max();
        std::FILE *_fd{nullptr};
        turbo::filesystem::path _file_path;
        turbo::FileOption _option;
        FileEventListener _listener;
    };
} // namespace turbo

#endif // TURBO_FILES_SEQUENTIAL_WRITE_FILE_H_
