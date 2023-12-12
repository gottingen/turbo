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
#ifndef TURBO_FILES_UTILITY_H_
#define TURBO_FILES_UTILITY_H_

#include <string>
#include <cstdio>
#include <tuple>
#include "turbo/files/file_option.h"
#include "turbo/files/filesystem.h"
#include "turbo/base/result_status.h"

namespace turbo {

    /**
     * @ingroup turbo_files
     * @brief FileUtility is a file io utility class.
     */
    class FileUtility {
    public:
        //
        // return file path and its extension:
        //
        // "mylog.txt" => ("mylog", ".txt")
        // "mylog" => ("mylog", "")
        // "mylog." => ("mylog.", "")
        // "/dir1/dir2/mylog.txt" => ("/dir1/dir2/mylog", ".txt")
        //
        // the starting dot in filenames is ignored (hidden files):
        //
        // ".mylog" => (".mylog". "")
        // "my_folder/.mylog" => ("my_folder/.mylog", "")
        // "my_folder/.mylog.txt" => ("my_folder/.mylog", ".txt")

        static std::tuple<std::string, std::string> split_by_extension(const std::string &fname);

        /**
         * @ingroup turbo_files_utility
         * @brief get file md5 sum.
         * @param path [input] file path
         * @param size [output] file size that is calculated.
         * @return md5 sum of the file and the status of the operation.
         * @note use the result of this function should check the status of the operation first.
         *      if the status is not ok, the result is invalid.
         * @code
         *     auto rs = FileUtility::md5_sum_file("test.txt");
         *     if (rs.ok()) {
         *          std::cout << rs.value() << std::endl;
         *          // do something else.
         *      } else {
         *           std::cout << rs.status().message() << std::endl;
         *           // do something else.
         *      }
         * @endcode
         */
        static turbo::ResultStatus<std::string> md5_sum_file(const std::string_view &path, int64_t *size = nullptr);

        /**
         * @ingroup turbo_files_utility
         * @brief get file sha1 sum.
         * @param path [input] file path
         * @param size [output] file size that is calculated.
         * @return sha1 sum of the file and the status of the operation.
         * @note use the result of this function should check the status of the operation first.
         *      if the status is not ok, the result is invalid.
         * @code
         *     auto rs = FileUtility::sha1_sum_file("test.txt");
         *     if (rs.ok()) {
         *          std::cout << rs.value() << std::endl;
         *          // do something else.
         *      } else {
         *           std::cout << rs.status().message() << std::endl;
         *           // do something else.
         *      }
         * @endcode
         */
        static turbo::ResultStatus<std::string> sha1_sum_file(const std::string_view &path, int64_t *size = nullptr);

        /**
         * @ingroup turbo_files_utility
         * @brief list files in the specified directory.
         *        If the directory does not exist, it will be returned with an error.
         *        If the directory is not a directory, it will be returned with an error.
         *        If the full_path is true, the result will be the full path of the file.
         *        eg : root_path = "/tmp", full_path = true, result = ["/tmp/file1", "/tmp/file2"]
         *        if the full_path is false, the result will be the file name.
         *        eg : root_path = "/tmp", full_path = false, result = ["file1", "file2"]
         * @param root_path directory path
         * @param result [output] file list
         * @param full_path [input] if true, the result will be the full path of the file.
         * @return the status of the operation.
         */
        static turbo::Status list_files(const std::string_view &root_path,std::vector<std::string> &result, bool full_path = true) noexcept;

        /**
         * @ingroup turbo_files_utility
         * @brief list directories in the specified directory.
         *        If the directory does not exist, it will be returned with an error.
         *        If the directory is not a directory, it will be returned with an error.
         *        If the full_path is true, the result will be the full path of the directory.
         *        eg : root_path = "/tmp", full_path = true, result = ["/tmp/dir1", "/tmp/dir2"]
         *        if the full_path is false, the result will be the directory name.
         *        eg : root_path = "/tmp", full_path = false, result = ["dir1", "dir2"]
         * @param root_path directory path
         * @param result [output] directory list
         * @param full_path [input] if true, the result will be the full path of the directory.
         * @return the status of the operation.
         */
        static turbo::Status list_directories(const std::string_view &root_path,std::vector<std::string> &result, bool full_path = true) noexcept;

        /**
         * @ingroup turbo_files_utility
         * @brief read file content.
         * @param file_path [input] file path
         * @param result [output] file content
         * @param append [input] if true, the file content will be appended to the result.
         * @return the status of the operation.
         */
        static turbo::Status read_file(const std::string_view &file_path, std::string &result, bool append = false) noexcept;

        /**
         * @ingroup turbo_files_utility
         * @brief write file content.
         * @param file_path [input] file path
         * @param content [input] file content
         * @param truncate [input] if true, the file will be truncated before write.
         * @return the status of the operation.
         */
        static turbo::Status write_file(const std::string_view &file_path, const std::string_view &content, bool truncate = true) noexcept;
    };
}  // namespace turbo

#endif // TURBO_FILES_UTILITY_H_
