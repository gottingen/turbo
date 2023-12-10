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
//

#ifndef TURBO_FILES_FIO_H_
#define TURBO_FILES_FIO_H_

#include <string>
#include <cstdio>
#include <tuple>
#include "turbo/files/file_option.h"
#include "turbo/files/filesystem.h"
#include "turbo/base/result_status.h"


namespace turbo {

     /**
      * @ingroup turbo_files_operation
      * @brief Fio is a file io utility class.
      *        It is Low-level operations on filesystem of the
      *        underlying operating system. we have Encapsulates
      *        some upper-level classes and interfaces. this is
      *        not recommended to use this class directly.
      */

    class Fio {
    public:
        /**
         * @brief file_open_write is a function to open a file for writing.
         * @param filename is the file name to open.
         * @param mode is the file open mode.
         * @param option is the file open option.
         * @return a ResultStatus<std::FILE *> object.
         */
        static turbo::ResultStatus<std::FILE *> file_open_write(const turbo::filesystem::path &filename, const std::string &mode,
                                                      const FileOption &option = FileOption());

        /**
         * @brief file_open_read is a function to open a file for reading.
         * @param filename is the file name to open.
         * @param mode is the file open mode.
         * @param option is the file open option.
         * @return a ResultStatus<std::FILE *> object.
         */
        static turbo::ResultStatus<std::FILE *> file_open_read(const turbo::filesystem::path &filename, const std::string &mode,
                                                                const FileOption &option = FileOption());

        /**
         * @brief file_open_append is a function to open a file for appending.
         *        this is used to get file size when the file is opened.
         * @param fp
         * @return
         */
        static turbo::ResultStatus<size_t> file_size(std::FILE *fp);

        /**
         * @brief file_size is a function to get the file size.
         *        this is linux version.
         * @param filename is the file name to get the size.
         * @return a ResultStatus<size_t> object.
         */
        static turbo::ResultStatus<size_t> file_size(int fd);

    };

#if defined(TURBO_PLATFORM_LINUX)
    typedef int FILE_HANDLER;
    static constexpr FILE_HANDLER INVALID_FILE_HANDLER = -1;
#elif defined(TURBO_PLATFORM_WINDOWS)

#endif

}  // namespace turbo

#endif  // TURBO_FILES_FIO_H_
