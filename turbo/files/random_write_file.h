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

    /**
     * @ingroup turbo_files
     * @brief RandomWriteFile is a file io utility class.
     * eg.
     * @code
     *      RandomWriteFile file;
     *      auto rs = file.open("test.txt");
     *      if (!rs.ok()) {
     *          std::cout << rs.status().message() << std::endl;
     *          // or throw exception.
     *          return;
     *      }
     *      std::string content = "hello world";
     *      // write content to file.
     *      rs = file.write(0, content);
     *      if (!rs.ok()) {
     *          std::cout << rs.status().message() << std::endl;
     *          // or throw exception.
     *          return;
     *      }
     *      // write content to file from offset 10.
     *      rs = file.write(10, content);
     *      if (!rs.ok()) {
     *          std::cout << rs.status().message() << std::endl;
     *          // or throw exception.
     *          return;
     *       }
     *       // write content to file from offset 10 and truncate file.
     *       rs = file.write(10, content, true);
     *       if (!rs.ok()) {
     *           std::cout << rs.status().message() << std::endl;
     *           // or throw exception.
     *           return;
     *       }
     *       // when write file recommend to call flush function.
     *       file.flush();
     *       // close file or use RAII,  it is recommended to call flush function before close file.
     *       file.close();
     * @endcode
     */
    class RandomWriteFile {
    public:

        RandomWriteFile() = default;

        explicit RandomWriteFile(const FileEventListener &listener);

        ///

        ~RandomWriteFile();

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
         * @param truncate truncate file if true, default is false.
         * @return the status of the operation. If the file is opened successfully, the status is OK.
         */
        [[nodiscard]] turbo::Status open(const turbo::filesystem::path &fname,
                           bool truncate = false);

        /**
         * @brief reopen file with path and option specified by user.
         *        The option can be set by set_option function. @see set_option.
         *        If the file does not exist, it will be created.
         *        If the file exists, it will be opened.
         *        If the file exists and the truncate option is true, the file will be truncated.
         * @param truncate truncate file if true, default is false.
         * @return the status of the operation. If the file is opened successfully, the status is OK.
         */
        [[nodiscard]] turbo::Status reopen(bool truncate = false);

        /**
         * @brief write file content from offset to the specified length.
         * @param offset [input] file offset
         * @param data [input] file content, can not be nullptr.
         * @param size [input] write length.
         * @param truncate [input] truncate file if true, default is false.
         *        If set to true, the file will be truncated to the length + offset.
         * @return the status of the operation.
         */
        [[nodiscard]] turbo::Status write(size_t offset,const char *data, size_t size, bool truncate = false);

        /**
         * @brief write file content from offset to the specified length.
         * @param offset [input] file offset
         * @param str [input] file content, can not be empty.
         * @param truncate [input] truncate file if true, default is false.
         *       If set to true, the file will be truncated to the str.size() + offset.
         * @return the status of the operation.
         */
        [[nodiscard]] turbo::Status write(size_t offset, std::string_view str, bool truncate = false) {
            return write(offset, str.data(), str.size(), truncate);
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
         * @return the status of the operation.
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
        std::FILE *_fp{nullptr};
        int        _fd{-1};
        turbo::filesystem::path _file_path;
        turbo::FileOption _option;
        FileEventListener _listener;
    };
}  // namespace turbo

#endif  // TURBO_FILES_RANDOM_WRITE_FILE_H_
