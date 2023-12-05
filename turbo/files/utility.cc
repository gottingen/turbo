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
#include "turbo/files/utility.h"
#include "turbo/files/sequential_read_file.h"
#include "turbo/files/sequential_write_file.h"
#include "turbo/crypto/md5.h"
#include "turbo/crypto/sha1.h"

namespace turbo {

    std::tuple<std::string, std::string> FileUtility::split_by_extension(const std::string &fname) {
        auto ext_index = fname.rfind('.');

        // no valid extension found - return whole path and empty string as
        // extension
        if (ext_index == std::string::npos || ext_index == 0 || ext_index == fname.size() - 1) {
            return std::make_tuple(fname, std::string());
        }

        // treat cases like "/etc/rc.d/somelogfile or "/abc/.hiddenfile"
        auto folder_index = fname.find_last_of("/");
        if (folder_index != std::string::npos && folder_index >= ext_index - 1) {
            return std::make_tuple(fname, std::string());
        }

        // finally - return a valid base and extension tuple
        return std::make_tuple(fname.substr(0, ext_index), fname.substr(ext_index));
    }

    turbo::ResultStatus<std::string> FileUtility::md5_sum_file(const std::string_view &path, int64_t *size) {
        static const size_t kBuffSize = 4096;
        turbo::SequentialReadFile file;
        auto rs = file.open(path);
        if (!rs.ok()) {
            return rs;
        }
        turbo::MD5 sum;
        size_t cnt = 0;
        std::error_code ec;
        auto len = turbo::filesystem::file_size(path, ec);
        if (ec) {
            return turbo::internal_error(ec.message());
        }
        std::string buf;
        buf.reserve(kBuffSize);
        do {
            buf.clear();
            auto r = file.read(&buf, kBuffSize);
            if (!r.ok()) {
                return r.status();
            }
            cnt += r.value();
            sum.process(buf);
        } while (cnt < len);
        if (size) {
            *size = cnt;
        }
        return sum.digest_hex();
    }

    turbo::ResultStatus<std::string> FileUtility::sha1_sum_file(const std::string_view &path, int64_t *size) {
        static const size_t kBuffSize = 4096;
        turbo::SequentialReadFile file;
        auto rs = file.open(path);
        if (!rs.ok()) {
            return rs;
        }
        turbo::SHA1 sum;
        size_t cnt = 0;
        std::error_code ec;
        auto len = turbo::filesystem::file_size(path, ec);
        if (ec) {
            return turbo::internal_error(ec.message());
        }
        std::string buf;
        buf.reserve(kBuffSize);
        do {
            buf.clear();
            auto r = file.read(&buf, kBuffSize);
            if (!r.ok()) {
                return r.status();
            }
            cnt += r.value();
            sum.process(buf);
        } while (cnt < len);
        if (size) {
            *size = cnt;
        }
        return sum.digest_hex();
    }

    turbo::Status FileUtility::list_files(const std::string_view &root_path,std::vector<std::string> &result, bool full_path) noexcept{
        std::error_code ec;
        turbo::filesystem::directory_iterator itr(root_path, ec);
        if(ec) {
            return turbo::make_status(ec.value(), "open directory error:{}", ec.message());
        }
        turbo::filesystem::directory_iterator end;
        for(;itr != end;++itr) {
            if(!itr->is_directory(ec)) {
                if(ec) {
                    return turbo::make_status(ec.value(), "test if file error:{}", ec.message());
                }
                if(full_path) {
                    result.emplace_back(itr->path().string());
                } else {
                    result.emplace_back(itr->path().filename());
                }
            }
        }
        return turbo::ok_status();
    }

    turbo::Status FileUtility::list_directories(const std::string_view &root_path,std::vector<std::string> &result, bool full_path) noexcept {
        std::error_code ec;
        turbo::filesystem::directory_iterator itr(root_path, ec);
        if(ec) {
            return turbo::make_status(ec.value(), "open directory error:{}", ec.message());
        }
        turbo::filesystem::directory_iterator end;
        for(;itr != end;++itr) {
            if(itr->is_directory(ec)) {
                if(ec) {
                    return turbo::make_status(ec.value(), "test if file error:{}", ec.message());
                }
                if(full_path) {
                    result.emplace_back(itr->path().string());
                } else {
                    result.emplace_back(itr->path().filename());
                }
            }
        }
        return turbo::ok_status();
    }

    turbo::Status FileUtility::read_file(const std::string_view &file_path, std::string &result, bool append) noexcept {
        if(!append) {
            result.clear();
        }
        SequentialReadFile file;
        auto rs = file.open(file_path);
        if(!rs.ok()) {
            return rs;
        }
        auto r = file.read(&result);
        if(!r.ok()) {
            return r.status();
        }
        file.close();
        return turbo::ok_status();
    }

    turbo::Status FileUtility::write_file(const std::string_view &file_path, const std::string_view &content, bool truncate) noexcept {
        SequentialWriteFile file;
        auto rs = file.open(file_path, truncate);
        if(!rs.ok()) {
            return rs;
        }

        rs = file.write(content);
        if(!rs.ok()) {
            return rs;
        }
        file.close();
        return turbo::ok_status();
    }
}  // namespace turbo
