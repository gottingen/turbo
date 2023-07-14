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

    class Fio {
    public:

        static turbo::ResultStatus<std::FILE *> file_open_write(const turbo::filesystem::path &filename, const std::string &mode,
                                                      const FileOption &option = FileOption());

        static turbo::ResultStatus<std::FILE *> file_open_read(const turbo::filesystem::path &filename, const std::string &mode,
                                                                const FileOption &option = FileOption());

        static turbo::ResultStatus<size_t> file_size(std::FILE *fp);

    };

}
#endif  // TURBO_FILES_FIO_H_
