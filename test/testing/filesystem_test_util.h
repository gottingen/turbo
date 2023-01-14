
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef TEST_TESTING_FILESYSTEM_TEST_UTIL_H_
#define TEST_TESTING_FILESYSTEM_TEST_UTIL_H_

#include "testing/sstream_workaround.h"
#include <turbo/base/profile.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <thread>


#if (defined(WIN32) || defined(_WIN32)) && !defined(__GNUC__)
#define NOMINMAX 1
#endif

#ifdef USE_STD_FS
#include <filesystem>
namespace fs {
using namespace std::filesystem;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using fstream = std::fstream;
}  // namespace fs
#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#ifdef _MSC_VER
#define IS_WCHAR_PATH
#endif

#ifdef WIN32
#define GHC_OS_WINDOWS
#endif

#else

#include "turbo/files/filesystem.h"
#endif

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#else

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Behaviour Switches (should match the config in ghc/filesystem.hpp):
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LWG #2682 disables the since then invalid use of the copy option create_symlinks on directories
#define TEST_LWG_2682_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LWG #2395 makes crate_directory/create_directories not emit an error if there is a regular
// file with that name, it is superceded by P1164R1, so only activate if really needed
// #define TEST_LWG_2935_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// LWG #2937 enforces that turbo::equivalent emits an error, if !turbo::exists(p1)||!exists(p2)
#define TEST_LWG_2937_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<typename TP>
std::time_t to_time_t(TP tp) {
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

template<typename TP>
TP from_time_t(std::time_t t) {
    using namespace std::chrono;
    auto sctp = system_clock::from_time_t(t);
    auto tp = time_point_cast<typename TP::duration>(sctp - system_clock::now() + TP::clock::now());
    return tp;
}


enum class TempOpt {
    none, change_path
};

class TemporaryDirectory {
public:
    TemporaryDirectory(TempOpt opt = TempOpt::none) {
        static auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        static auto rng = std::bind(std::uniform_int_distribution<int>(0, 35), std::mt19937(
                static_cast<unsigned int>(seed) ^ static_cast<unsigned int>(reinterpret_cast<ptrdiff_t>(&opt))));
        std::string filename;
        do {
            filename = "test_";
            for (int i = 0; i < 8; ++i) {
                filename += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rng()];
            }
            _path = turbo::canonical(turbo::temp_directory_path()) / filename;
        } while (turbo::exists(_path));
        turbo::create_directories(_path);
        if (opt == TempOpt::change_path) {
            _orig_dir = turbo::current_path();
            turbo::current_path(_path);
        }
    }

    ~TemporaryDirectory() {
        if (!_orig_dir.empty()) {
            turbo::current_path(_orig_dir);
        }
        turbo::remove_all(_path);
    }

    const turbo::file_path &path() const { return _path; }

private:
    turbo::file_path _path;
    turbo::file_path _orig_dir;
};

static void generateFile(const turbo::file_path &pathname, int withSize = -1) {
    turbo::ofstream outfile(pathname);
    if (withSize < 0) {
        outfile << "Hello world!" << std::endl;
    } else {
        outfile << std::string(size_t(withSize), '*');
    }
}

#ifdef ABEL_PLATFORM_WINDOWS
inline bool isWow64Proc()
{
    typedef BOOL(WINAPI * IsWow64Process_t)(HANDLE, PBOOL);
    BOOL bIsWow64 = FALSE;
    auto fnIsWow64Process = (IsWow64Process_t)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
    if (nullptr != fnIsWow64Process) {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
            bIsWow64 = FALSE;
        }
    }
    return bIsWow64 == TRUE;
}

static bool is_symlink_creation_supported()
{
    bool result = true;
    HKEY key;
    REGSAM flags = KEY_READ;
#ifdef _WIN64
    flags |= KEY_WOW64_64KEY;
#elif defined(KEY_WOW64_64KEY)
    if (isWow64Proc()) {
        flags |= KEY_WOW64_64KEY;
    }
    else {
        flags |= KEY_WOW64_32KEY;
    }
#else
    result = false;
#endif
    if (result) {
        auto err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, flags, &key);
        if (err == ERROR_SUCCESS) {
            DWORD val = 0, size = sizeof(DWORD);
            err = RegQueryValueExW(key, L"AllowDevelopmentWithoutDevLicense", 0, nullptr, reinterpret_cast<LPBYTE>(&val), &size);
            RegCloseKey(key);
            if (err != ERROR_SUCCESS) {
                result = false;
            }
            else {
                result = (val != 0);
            }
        }
        else {
            result = false;
        }
    }
    if (!result) {
        std::clog << "Warning: Symlink creation not supported." << std::endl;
    }
    return result;
}
#else

static bool is_symlink_creation_supported() {
    return true;
}

#endif

static bool has_host_root_name_support() {
    return turbo::file_path("//host").has_root_name();
}

template<class T>
class TestAllocator : public std::allocator<T> {
public:
    using value_type = T;

    TestAllocator() noexcept {}

    template<class U>
    TestAllocator(TestAllocator<U> const &) noexcept {
    }

    value_type *allocate(std::size_t n) { return static_cast<value_type *>(::operator new(n * sizeof(value_type))); }

    void deallocate(value_type *p, std::size_t) noexcept { ::operator delete(p); }


    template<class U>
    struct rebind {
        using other = TestAllocator<U>;
    };
};

template<class T, class U>
bool operator==(TestAllocator<T> const &, TestAllocator<U> const &) noexcept {
    return true;
}

template<class T, class U>
bool operator!=(TestAllocator<T> const &x, TestAllocator<U> const &y) noexcept {
    return !(x == y);
}

#endif //TEST_TESTING_FILESYSTEM_TEST_UTIL_H_
