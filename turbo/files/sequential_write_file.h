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

namespace turbo {

class SequentialWriteFile {
public:
  SequentialWriteFile() = default;
  ~SequentialWriteFile();

  turbo::Status open(const turbo::filesystem::path &fname,
                     bool truncate = false);

  turbo::Status reopen(const turbo::filesystem::path &fname,
                       bool truncate = false);

  turbo::ResultStatus<ssize_t> append(const char *data, size_t size);

  turbo::ResultStatus<ssize_t> append(std::string_view str) {
    return append(str.data(), str.size());
  }

  size_t have_write() const;

  turbo::ResultStatus<size_t> size() const;

  void close();

  void flush();

  const turbo::filesystem::path &file_path() const;

private:
  static const size_t npos = std::numeric_limits<size_t>::max();
  int _fd{-1};
  turbo::filesystem::path _path;
  size_t _has_write{0};
};
} // namespace turbo

#endif // TURBO_FILES_SEQUENTIAL_WRITE_FILE_H_
