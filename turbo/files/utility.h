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

    };
}  // namespace turbo

#endif // TURBO_FILES_UTILITY_H_
