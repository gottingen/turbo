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

#include "turbo/files/fio.h"
#include "turbo/platform/port.h"

namespace turbo {

    turbo::ResultStatus<std::FILE *>
    Fio::file_open(const turbo::filesystem::path &filename, const std::string &mode, const FileOption &option) {
        std::FILE *fp = nullptr;
#ifdef TURBO_PLATFORM_WINDOWS
#ifdef TURBO_WCHAR_FILENAMES
        fp = ::_wfsopen((filename.c_str()), mode.c_str(), _SH_DENYNO);
#else
        fp = ::_fsopen((filename.c_str()), mode.c_str(), _SH_DENYNO);
#endif
#if defined(TURBO_PREVENT_CHILD_FD)
        if (fp != nullptr) {
            auto file_handle = reinterpret_cast<HANDLE>(_get_osfhandle(::_fileno(fp)));
            if (!::SetHandleInformation(file_handle, HANDLE_FLAG_INHERIT, 0))
            {
                ::fclose(fp);
                fp = nullptr;
            }
        }
#endif
#else // unix
        if (option.prevent_child) {
            const int mode_flag = (mode == "ab") ? O_APPEND : O_TRUNC;
            const int fd = ::open((filename.c_str()), O_CREAT | O_WRONLY | O_CLOEXEC | mode_flag, mode_t(0644));
            if (fd == -1) {
                return turbo::ErrnoToStatus(errno, "");
            }
            fp = ::fdopen(fd, mode.c_str());
            if (fp == nullptr) {
                ::close(fd);
            }
        } else {
            fp = ::fopen((filename.c_str()), mode.c_str());
        }
#endif
        if (fp == nullptr) {
            return turbo::ErrnoToStatus(errno, "");
        }
        return fp;
    }

    turbo::ResultStatus<size_t> Fio::file_size(std::FILE *fp) {
        if (fp == nullptr) {
            return turbo::InvalidArgumentError("Failed getting file size. fp is null");
        }
#if defined(_WIN32) && !defined(__CYGWIN__)
        int fd = ::_fileno(f);
#    if defined(_WIN64) // 64 bits
                __int64 ret = ::_filelengthi64(fd);
                if (ret >= 0)
                {
                    return static_cast<size_t>(ret);
                }

#    else // windows 32 bits
                long ret = ::_filelength(fd);
                if (ret >= 0)
                {
                    return static_cast<size_t>(ret);
                }
#    endif

#else // unix
// OpenBSD and AIX doesn't compile with :: before the fileno(..)
#    if defined(__OpenBSD__) || defined(_AIX)
        int fd = fileno(f);
#    else
        int fd = ::fileno(fp);
#    endif
// 64 bits(but not in osx or cygwin, where fstat64 is deprecated)
#    if (defined(__linux__) || defined(__sun) || defined(_AIX)) && (defined(__LP64__) || defined(_LP64))
        struct stat64 st;
        if (::fstat64(fd, &st) == 0) {
            return static_cast<size_t>(st.st_size);
        }
#    else // other unix or linux 32 bits or cygwin
        struct stat st;
                if (::fstat(fd, &st) == 0)
                {
                    return static_cast<size_t>(st.st_size);
                }
#    endif
#endif
        return turbo::ErrnoToStatus(errno, "Failed getting file size from fd");
    }

}  // namespace turbo
