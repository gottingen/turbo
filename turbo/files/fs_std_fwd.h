//---------------------------------------------------------------------------------------
//
// turbo::filesystem - A C++17-like filesystem implementation for C++11/C++14
//
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2018, Steffen Schümann <s.schuemann@pobox.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//---------------------------------------------------------------------------------------
// fs_std_fwd.hpp - The forwarding header for the header/implementation seperated usage of
//                  turbo::filesystem that uses std::filesystem if it detects it.
// This file can be include at any place, where fs::filesystem api is needed while
// not bleeding implementation details (e.g. system includes) into the global namespace,
// as long as one cpp includes fs_std_impl.hpp to deliver the matching implementations.
//---------------------------------------------------------------------------------------
#ifndef TURBO_FILES_FS_STD_FWD_H_
#define TURBO_FILES_FS_STD_FWD_H_
#if defined(__APPLE__)

#include <Availability.h>

#endif
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || (defined(__cplusplus) && __cplusplus >= 201703L)) && defined(__has_include)
#if __has_include(<filesystem>) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500)
#define TURBO_USE_STD_FS
#include <filesystem>
namespace fs {
using namespace std::filesystem;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using fstream = std::fstream;
}
#endif
#endif
#ifndef TURBO_USE_STD_FS
//#define TURBO_WIN_DISABLE_WSTRING_STORAGE_TYPE
#define TURBO_FILESYSTEM_FWD

#include "turbo/files/filesystem.h"

namespace fs {
    using namespace turbo::filesystem;
    using ifstream = turbo::filesystem::ifstream;
    using ofstream = turbo::filesystem::ofstream;
    using fstream = turbo::filesystem::fstream;
}
#endif
#endif  // TURBO_FILES_FS_STD_FWD_H_

