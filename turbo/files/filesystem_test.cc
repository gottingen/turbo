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
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
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
#include "turbo/log/turbo_log.h"
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
} // namespace fs
#ifdef __GNUC__
#define GCC_VERSION                                                            \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif
#ifdef _MSC_VER
#define IS_WCHAR_PATH
#endif
#ifdef WIN32
#define TURBO_PLATFORM_WINDOWS
#endif
#else
#ifdef TURBO_FILESYSTEM_FWD_TEST
#include "turbo/files/fs_fwd.h"
#else
#include "turbo/files/filesystem.h"
#endif
namespace fs {
using namespace turbo::filesystem;
using ifstream = turbo::filesystem::ifstream;
using ofstream = turbo::filesystem::ofstream;
using fstream = turbo::filesystem::fstream;
} // namespace fs
#endif

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#ifndef TURBO_FILESYSTEM_FWD_TEST
#define CATCH_CONFIG_MAIN
#endif
#include "gtest/gtest.h"

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - -
// Behaviour Switches (should match the config in turbo/filesystem.hpp):
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - -
// LWG #2682 disables the since then invalid use of the copy option
// create_symlinks on directories
#define TEST_LWG_2682_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - -
// LWG #2395 makes crate_directory/create_directories not emit an error if there
// is a regular file with that name, it is superceded by P1164R1, so only
// activate if really needed #define TEST_LWG_2935_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - -
// LWG #2937 enforces that fs::equivalent emits an error, if
// !fs::exists(p1)||!exists(p2)
#define TEST_LWG_2937_BEHAVIOUR
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//- - - - - - - - -

template <typename TP> std::time_t to_time_t(TP tp) {
  using namespace std::chrono;
  auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() +
                                                      system_clock::now());
  return system_clock::to_time_t(sctp);
}

template <typename TP> TP from_time_t(std::time_t t) {
  using namespace std::chrono;
  auto sctp = system_clock::from_time_t(t);
  auto tp = time_point_cast<typename TP::duration>(sctp - system_clock::now() +
                                                   TP::clock::now());
  return tp;
}

/*
namespace Catch {
template <>
struct StringMaker<fs::path>
{
    static std::string convert(fs::path const& value) { return '"' +
value.string() + '"'; }
};

template <>
struct StringMaker<fs::perms>
{
    static std::string convert(fs::perms const& value) { return
std::to_string(static_cast<unsigned int>(value)); }
};

template <>
struct StringMaker<fs::file_status>
{
    static std::string convert(fs::file_status const& value) {
        return std::string("[") + std::to_string(static_cast<unsigned
int>(value.type())) + "," + std::to_string(static_cast<unsigned
int>(value.permissions())) + "]";
    }
};

#ifdef __cpp_lib_char8_t
template <>
struct StringMaker<char8_t>
{
    static std::string convert(char8_t const& value) { return
std::to_string(static_cast<unsigned int>(value)); }
};
#endif

template <>
struct StringMaker<fs::file_time_type>
{
    static std::string convert(fs::file_time_type const& value)
    {
        std::time_t t = to_time_t(value);
        std::tm* ptm = std::localtime(&t);
        std::ostringstream os;
        if (ptm) {
            std::tm ttm = *ptm;
            os << std::put_time(&ttm, "%Y-%m-%d %H:%M:%S");
        }
        else {
            os << "(invalid-time)";
        }
        return os.str();
    }
};
}  // namespace Catch
*/
enum class TempOpt { none, change_path };
class TemporaryDirectory {
public:
  TemporaryDirectory(TempOpt opt = TempOpt::none) {
    static auto seed =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    static auto rng =
        std::bind(std::uniform_int_distribution<int>(0, 35),
                  std::mt19937(static_cast<unsigned int>(seed) ^
                               static_cast<unsigned int>(
                                   reinterpret_cast<ptrdiff_t>(&opt))));
    std::string filename;
    do {
      filename = "test_";
      for (int i = 0; i < 8; ++i) {
        filename += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rng()];
      }
      _path = fs::canonical(fs::temp_directory_path()) / filename;
    } while (fs::exists(_path));
    fs::create_directories(_path);
    if (opt == TempOpt::change_path) {
      _orig_dir = fs::current_path();
      fs::current_path(_path);
    }
  }

  ~TemporaryDirectory() {
    if (!_orig_dir.empty()) {
      fs::current_path(_orig_dir);
    }
    fs::remove_all(_path);
  }

  const fs::path &path() const { return _path; }

private:
  fs::path _path;
  fs::path _orig_dir;
};

static void generateFile(const fs::path &pathname, int withSize = -1) {
  fs::ofstream outfile(pathname);
  if (withSize < 0) {
    outfile << "Hello world!" << std::endl;
  } else {
    outfile << std::string(size_t(withSize), '*');
  }
}

#ifdef TURBO_PLATFORM_WINDOWS
inline bool isWow64Proc() {
  typedef BOOL(WINAPI * IsWow64Process_t)(HANDLE, PBOOL);
  BOOL bIsWow64 = FALSE;
  auto fnIsWow64Process = (IsWow64Process_t)GetProcAddress(
      GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
  if (NULL != fnIsWow64Process) {
    if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
      bIsWow64 = FALSE;
    }
  }
  return bIsWow64 == TRUE;
}

static bool is_symlink_creation_supported() {
  bool result = true;
  HKEY key;
  REGSAM flags = KEY_READ;
#ifdef _WIN64
  flags |= KEY_WOW64_64KEY;
#elif defined(KEY_WOW64_64KEY)
  if (isWow64Proc()) {
    flags |= KEY_WOW64_64KEY;
  } else {
    flags |= KEY_WOW64_32KEY;
  }
#else
  result = false;
#endif
  if (result) {
    auto err = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0,
        flags, &key);
    if (err == ERROR_SUCCESS) {
      DWORD val = 0, size = sizeof(DWORD);
      err = RegQueryValueExW(key, L"AllowDevelopmentWithoutDevLicense", 0, NULL,
                             reinterpret_cast<LPBYTE>(&val), &size);
      RegCloseKey(key);
      if (err != ERROR_SUCCESS) {
        result = false;
      } else {
        result = (val != 0);
      }
    } else {
      result = false;
    }
  }
  if (!result) {
    std::clog << "Warning: Symlink creation not supported." << std::endl;
  }
  return result;
}
#else
static bool is_symlink_creation_supported() { return true; }
#endif

static bool has_host_root_name_support() {
  return fs::path("//host").has_root_name();
}

template <class T> class TestAllocator {
public:
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using difference_type = ptrdiff_t;
  using size_type = size_t;
  TestAllocator() noexcept {}
  template <class U> TestAllocator(TestAllocator<U> const &) noexcept {}
  value_type *allocate(std::size_t n) {
    return static_cast<value_type *>(::operator new(n * sizeof(value_type)));
  }
  void deallocate(value_type *p, std::size_t) noexcept { ::operator delete(p); }
  template <class U> struct rebind {
    typedef TestAllocator<U> other;
  };
};

template <class T, class U>
bool operator==(TestAllocator<T> const &, TestAllocator<U> const &) noexcept {
  return true;
}

template <class T, class U>
bool operator!=(TestAllocator<T> const &x, TestAllocator<U> const &y) noexcept {
  return !(x == y);
}

TEST(TemporaryDirectory, fsTestTempdir) {
  fs::path tempPath;
  {
    TemporaryDirectory t;
    tempPath = t.path();
    ASSERT_TRUE(fs::exists(fs::path(t.path())));
    ASSERT_TRUE(fs::is_directory(t.path()));
  }
  ASSERT_TRUE(!fs::exists(tempPath));
}

#ifdef TURBO_FILESYSTEM_VERSION
TEST(Filesystem, detail_utf8) {
  ASSERT_TRUE(fs::detail::fromUtf8<std::wstring>("foobar").length() == 6);
  ASSERT_TRUE(fs::detail::fromUtf8<std::wstring>("foobar") == L"foobar");
  ASSERT_TRUE(fs::detail::fromUtf8<std::wstring>(u8"föobar").length() == 6);
  ASSERT_TRUE(fs::detail::fromUtf8<std::wstring>(u8"föobar") == L"föobar");

  ASSERT_TRUE(fs::detail::toUtf8(std::wstring(L"foobar")).length() == 6);
  ASSERT_TRUE(fs::detail::toUtf8(std::wstring(L"foobar")) == "foobar");
  ASSERT_TRUE(fs::detail::toUtf8(std::wstring(L"föobar")).length() == 7);
  // ASSERT_TRUE(fs::detail::toUtf8(std::wstring(L"föobar")) == u8"föobar");

#ifdef TURBO_RAISE_UNICODE_ERRORS
  ASSERT_THROW(
      fs::detail::fromUtf8<std::u16string>(std::string("\xed\xa0\x80")),
      fs::filesystem_error);
  ASSERT_THROW(fs::detail::fromUtf8<std::u16string>(std::string("\xc3")),
               fs::filesystem_error);
#else
  ASSERT_TRUE(std::u16string(2, 0xfffd) == fs::detail::fromUtf8<std::u16string>(
                                               std::string("\xed\xa0\x80")));
  ASSERT_TRUE(std::u16string(1, 0xfffd) ==
              fs::detail::fromUtf8<std::u16string>(std::string("\xc3")));
#endif
}

TEST(fs_utf, detail_utf8) {
  std::string t;
  ASSERT_TRUE(std::string("\xc3\xa4/\xe2\x82\xac\xf0\x9d\x84\x9e") ==
              fs::detail::toUtf8(std::u16string(u"\u00E4/\u20AC\U0001D11E")));
#ifdef TURBO_RAISE_UNICODE_ERRORS
  ASSERT_THROW(fs::detail::toUtf8(std::u16string(1, 0xd800)),
               fs::filesystem_error);
  ASSERT_THROW(fs::detail::appendUTF8(t, 0x200000), fs::filesystem_error);
#else
  ASSERT_TRUE(std::string("\xEF\xBF\xBD") ==
              fs::detail::toUtf8(std::u16string(1, 0xd800)));
  fs::detail::appendUTF8(t, 0x200000);
  ASSERT_TRUE(std::string("\xEF\xBF\xBD") == t);
#endif
}
#endif

TEST(Filesystem, generic) {
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path::preferred_separator == '\\');
#else
  ASSERT_TRUE(fs::path::preferred_separator == '/');
#endif
}

#ifndef TURBO_PLATFORM_WINDOWS
TEST(Filesystem, path_gen) {
  if (!has_host_root_name_support()) {
    TURBO_LOG(WARNING) << "This implementation doesn't support "
                          "path(\"//host\").has_root_name() == true [C++17 "
                          "30.12.8.1 par. 4] on this platform, tests based on "
                          "this are skipped. (Should be okay.)";
  }
}
#endif

TEST(Filesystem, construct) {
  ASSERT_TRUE("/usr/local/bin" == fs::path("/usr/local/bin").generic_string());
  std::string str = "/usr/local/bin";
#if defined(__cpp_lib_char8_t) && !defined(TURBO_FILESYSTEM_ENFORCE_CPP17_API)
  std::u8string u8str = u8"/usr/local/bin";
#endif
  std::u16string u16str = u"/usr/local/bin";
  std::u32string u32str = U"/usr/local/bin";
#if defined(__cpp_lib_char8_t) && !defined(TURBO_FILESYSTEM_ENFORCE_CPP17_API)
  ASSERT_TRUE(u8str == fs::path(u8str).generic_u8string());
#endif
  ASSERT_TRUE(u16str == fs::path(u16str).generic_u16string());
  ASSERT_TRUE(u32str == fs::path(u32str).generic_u32string());
  ASSERT_TRUE(str == fs::path(str, fs::path::format::generic_format));
  ASSERT_TRUE(str == fs::path(str.begin(), str.end()));
  ASSERT_TRUE(fs::path(std::wstring(3, 67)) == "CCC");
#if defined(__cpp_lib_char8_t) && !defined(TURBO_FILESYSTEM_ENFORCE_CPP17_API)
  ASSERT_TRUE(str == fs::path(u8str.begin(), u8str.end()));
#endif
  ASSERT_TRUE(str == fs::path(u16str.begin(), u16str.end()));
  ASSERT_TRUE(str == fs::path(u32str.begin(), u32str.end()));
#ifdef TURBO_FILESYSTEM_VERSION
  ASSERT_TRUE(fs::path("///foo/bar") == "/foo/bar");
  ASSERT_TRUE(fs::path("//foo//bar") == "//foo/bar");
#endif
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE("\\usr\\local\\bin" == fs::path("/usr/local/bin"));
  ASSERT_TRUE("C:\\usr\\local\\bin" == fs::path("C:\\usr\\local\\bin"));
#else
  ASSERT_TRUE("/usr/local/bin" == fs::path("/usr/local/bin"));
#endif
  if (has_host_root_name_support()) {
    ASSERT_TRUE("//host/foo/bar" == fs::path("//host/foo/bar"));
  }

#if !defined(TURBO_PLATFORM_WINDOWS) &&                                        \
    !(defined(__GLIBCXX__) &&                                                  \
      !(defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 8))) &&              \
    !defined(USE_STD_FS)
  std::locale loc;
  bool testUTF8Locale = false;
  try {
    if (const char *lang = std::getenv("LANG")) {
      loc = std::locale(lang);
    } else {
      loc = std::locale("en_US.UTF-8");
    }
    std::string name = loc.name();
    if (name.length() > 5 && (name.substr(name.length() - 5) == "UTF-8" ||
                              name.substr(name.length() - 5) == "utf-8")) {
      testUTF8Locale = true;
    }
  } catch (std::runtime_error &) {
    TURBO_LOG(WARNING) << "Couldn't create an UTF-8 locale!";
  }
  if (testUTF8Locale) {
    ASSERT_TRUE("/usr/local/bin" == fs::path("/usr/local/bin", loc));
    ASSERT_TRUE(str == fs::path(str.begin(), str.end(), loc));
    ASSERT_TRUE(str == fs::path(u16str.begin(), u16str.end(), loc));
    ASSERT_TRUE(str == fs::path(u32str.begin(), u32str.end(), loc));
  }
#endif
}

TEST(FilesystemPath, assign) {
  fs::path p1{"/foo/bar"};
  fs::path p2{"/usr/local"};
  fs::path p3;
  p3 = p1;
  ASSERT_TRUE(p1 == p3);
  p3 = fs::path{"/usr/local"};
  ASSERT_TRUE(p2 == p3);
  p3 = fs::path{L"/usr/local"};
  ASSERT_TRUE(p2 == p3);
  p3.assign(L"/usr/local");
  ASSERT_TRUE(p2 == p3);
#if defined(IS_WCHAR_PATH) || defined(GHC_USE_WCHAR_T)
  p3 = fs::path::string_type{L"/foo/bar"};
  ASSERT_TRUE(p1 == p3);
  p3.assign(fs::path::string_type{L"/usr/local"});
  ASSERT_TRUE(p2 == p3);
#else
  p3 = fs::path::string_type{"/foo/bar"};
  ASSERT_TRUE(p1 == p3);
  p3.assign(fs::path::string_type{"/usr/local"});
  ASSERT_TRUE(p2 == p3);
#endif
  p3 = std::u16string(u"/foo/bar");
  ASSERT_TRUE(p1 == p3);
  p3 = U"/usr/local";
  ASSERT_TRUE(p2 == p3);
  p3.assign(std::u16string(u"/foo/bar"));
  ASSERT_TRUE(p1 == p3);
  std::string s{"/usr/local"};
  p3.assign(s.begin(), s.end());
  ASSERT_TRUE(p2 == p3);
}

TEST(FilesystemPath, append) {
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("foo") / "c:/bar" == "c:/bar");
  ASSERT_TRUE(fs::path("foo") / "c:" == "c:");
  ASSERT_TRUE(fs::path("c:") / "" == "c:");
  ASSERT_TRUE(fs::path("c:foo") / "/bar" == "c:/bar");
  ASSERT_TRUE(fs::path("c:foo") / "c:bar" == "c:foo/bar");
#else
  ASSERT_TRUE(fs::path("foo") / "" == "foo/");
  ASSERT_TRUE(fs::path("foo") / "/bar" == "/bar");
  ASSERT_TRUE(fs::path("/foo") / "/" == "/");
  if (has_host_root_name_support()) {
    ASSERT_TRUE(fs::path("//host/foo") / "/bar" == "/bar");
    ASSERT_TRUE(fs::path("//host") / "/" == "//host/");
    ASSERT_TRUE(fs::path("//host/foo") / "/" == "/");
  }
#endif
  ASSERT_TRUE(fs::path("/foo/bar") / "some///other" == "/foo/bar/some/other");
  fs::path p1{"/tmp/test"};
  fs::path p2{"foobar.txt"};
  fs::path p3 = p1 / p2;
  ASSERT_TRUE("/tmp/test/foobar.txt" == p3);
  // TODO: append(first, last)
}

TEST(FilesystemPath, concat) {
  ASSERT_TRUE((fs::path("foo") += fs::path("bar")) == "foobar");
  ASSERT_TRUE((fs::path("foo") += fs::path("/bar")) == "foo/bar");

  ASSERT_TRUE((fs::path("foo") += std::string("bar")) == "foobar");
  ASSERT_TRUE((fs::path("foo") += std::string("/bar")) == "foo/bar");

  ASSERT_TRUE((fs::path("foo") += "bar") == "foobar");
  ASSERT_TRUE((fs::path("foo") += "/bar") == "foo/bar");
  ASSERT_TRUE((fs::path("foo") += L"bar") == "foobar");
  ASSERT_TRUE((fs::path("foo") += L"/bar") == "foo/bar");

  ASSERT_TRUE((fs::path("foo") += 'b') == "foob");
  ASSERT_TRUE((fs::path("foo") += '/') == "foo/");
  ASSERT_TRUE((fs::path("foo") += L'b') == "foob");
  ASSERT_TRUE((fs::path("foo") += L'/') == "foo/");

  ASSERT_TRUE((fs::path("foo") += std::string("bar")) == "foobar");
  ASSERT_TRUE((fs::path("foo") += std::string("/bar")) == "foo/bar");

  ASSERT_TRUE((fs::path("foo") += std::u16string(u"bar")) == "foobar");
  ASSERT_TRUE((fs::path("foo") += std::u16string(u"/bar")) == "foo/bar");

  ASSERT_TRUE((fs::path("foo") += std::u32string(U"bar")) == "foobar");
  ASSERT_TRUE((fs::path("foo") += std::u32string(U"/bar")) == "foo/bar");

  ASSERT_TRUE(fs::path("foo").concat("bar") == "foobar");
  ASSERT_TRUE(fs::path("foo").concat("/bar") == "foo/bar");
  ASSERT_TRUE(fs::path("foo").concat(L"bar") == "foobar");
  ASSERT_TRUE(fs::path("foo").concat(L"/bar") == "foo/bar");
  std::string bar = "bar";
  ASSERT_TRUE(fs::path("foo").concat(bar.begin(), bar.end()) == "foobar");
#ifndef USE_STD_FS
  ASSERT_TRUE((fs::path("/foo/bar") += "/some///other") ==
              "/foo/bar/some/other");
#endif
  // TODO: contat(first, last)
}

TEST(FilesystemPath, modifiers) {
  fs::path p = fs::path("/foo/bar");
  p.clear();
  ASSERT_TRUE(p == "");

  // make_preferred() is a no-op
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("foo\\bar") == "foo/bar");
  ASSERT_TRUE(fs::path("foo\\bar").make_preferred() == "foo/bar");
#else
  ASSERT_TRUE(fs::path("foo\\bar") == "foo\\bar");
  ASSERT_TRUE(fs::path("foo\\bar").make_preferred() == "foo\\bar");
#endif
  ASSERT_TRUE(fs::path("foo/bar").make_preferred() == "foo/bar");

  ASSERT_TRUE(fs::path("foo/bar").remove_filename() == "foo/");
  ASSERT_TRUE(fs::path("foo/").remove_filename() == "foo/");
  ASSERT_TRUE(fs::path("/foo").remove_filename() == "/");
  ASSERT_TRUE(fs::path("/").remove_filename() == "/");

  ASSERT_TRUE(fs::path("/foo").replace_filename("bar") == "/bar");
  ASSERT_TRUE(fs::path("/").replace_filename("bar") == "/bar");
  ASSERT_TRUE(fs::path("/foo").replace_filename("b//ar") == "/b/ar");

  ASSERT_TRUE(fs::path("/foo/bar.txt").replace_extension("odf") ==
              "/foo/bar.odf");
  ASSERT_TRUE(fs::path("/foo/bar.txt").replace_extension() == "/foo/bar");
  ASSERT_TRUE(fs::path("/foo/bar").replace_extension("odf") == "/foo/bar.odf");
  ASSERT_TRUE(fs::path("/foo/bar").replace_extension(".odf") == "/foo/bar.odf");
  ASSERT_TRUE(fs::path("/foo/bar.").replace_extension(".odf") ==
              "/foo/bar.odf");
  ASSERT_TRUE(fs::path("/foo/bar/").replace_extension("odf") ==
              "/foo/bar/.odf");

  fs::path p1 = "foo";
  fs::path p2 = "bar";
  p1.swap(p2);
  ASSERT_TRUE(p1 == "bar");
  ASSERT_TRUE(p2 == "foo");
}

TEST(FilesystemPath, obs) {
#ifdef TURBO_PLATFORM_WINDOWS
#if defined(IS_WCHAR_PATH) || defined(GHC_USE_WCHAR_T)
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").native() ==
              fs::path::string_type(L"\u00E4\\\u20AC"));
  // ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").string() ==
  // std::string("ä\\€")); // MSVCs returns local DBCS encoding
#else
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").native() ==
              fs::path::string_type("\xc3\xa4\\\xe2\x82\xac"));
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").string() ==
              std::string("\xc3\xa4\\\xe2\x82\xac"));
  ASSERT_TRUE(!::strcmp(fs::u8path("\xc3\xa4\\\xe2\x82\xac").c_str(),
                        "\xc3\xa4\\\xe2\x82\xac"));
  ASSERT_TRUE((std::string)fs::u8path("\xc3\xa4\\\xe2\x82\xac") ==
              std::string("\xc3\xa4\\\xe2\x82\xac"));
#endif
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").wstring() ==
              std::wstring(L"\u00E4\\\u20AC"));
#if defined(__cpp_lib_char8_t) && !defined(TURBO_FILESYSTEM_ENFORCE_CPP17_API)
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").u8string() ==
              std::u8string(u8"\u00E4\\\u20AC"));
#else
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").u8string() ==
              std::string("\xc3\xa4\\\xe2\x82\xac"));
#endif
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").u16string() ==
              std::u16string(u"\u00E4\\\u20AC"));
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").u32string() ==
              std::u32string(U"\U000000E4\\\U000020AC"));
#else
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").native() ==
              fs::path::string_type("\xc3\xa4/\xe2\x82\xac"));
  ASSERT_TRUE(!::strcmp(fs::u8path("\xc3\xa4/\xe2\x82\xac").c_str(),
                        "\xc3\xa4/\xe2\x82\xac"));
  ASSERT_TRUE((std::string)fs::u8path("\xc3\xa4/\xe2\x82\xac") ==
              std::string("\xc3\xa4/\xe2\x82\xac"));
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").string() ==
              std::string("\xc3\xa4/\xe2\x82\xac"));
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").wstring() ==
              std::wstring(L"ä/€"));
#if defined(__cpp_lib_char8_t) && !defined(TURBO_FILESYSTEM_ENFORCE_CPP17_API)
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").u8string() ==
              std::u8string(u8"\xc3\xa4/\xe2\x82\xac"));
#else
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").u8string() ==
              std::string("\xc3\xa4/\xe2\x82\xac"));
#endif
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").u16string() ==
              std::u16string(u"\u00E4/\u20AC"));
  TURBO_LOG(INFO)
      << "This check might fail on GCC8 (with \"Illegal byte sequence\") due "
         "to not detecting the valid unicode codepoint U+1D11E.";
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac\xf0\x9d\x84\x9e").u16string() ==
              std::u16string(u"\u00E4/\u20AC\U0001D11E"));
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").u32string() ==
              std::u32string(U"\U000000E4/\U000020AC"));
#endif
}

TEST(FilesystemPath, generic_obs) {
#ifdef TURBO_PLATFORM_WINDOWS
#ifndef IS_WCHAR_PATH
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").generic_string() ==
              std::string("\xc3\xa4/\xe2\x82\xac"));
#endif
#ifndef USE_STD_FS
  auto t =
      fs::u8path("\xc3\xa4\\\xe2\x82\xac")
          .generic_string<char, std::char_traits<char>, TestAllocator<char>>();
  ASSERT_TRUE(t.c_str() == std::string("\xc3\xa4/\xe2\x82\xac"));
#endif
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").generic_wstring() ==
              std::wstring(L"\U000000E4/\U000020AC"));
#if defined(__cpp_lib_char8_t) && !defined(TURBO_FILESYSTEM_ENFORCE_CPP17_API)
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").generic_u8string() ==
              std::u8string(u8"\u00E4/\u20AC"));
#else
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").generic_u8string() ==
              std::string("\xc3\xa4/\xe2\x82\xac"));
#endif
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").generic_u16string() ==
              std::u16string(u"\u00E4/\u20AC"));
  ASSERT_TRUE(fs::u8path("\xc3\xa4\\\xe2\x82\xac").generic_u32string() ==
              std::u32string(U"\U000000E4/\U000020AC"));
#else
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").generic_string() ==
              std::string("\xc3\xa4/\xe2\x82\xac"));
#ifndef USE_STD_FS
  auto t =
      fs::u8path("\xc3\xa4/\xe2\x82\xac")
          .generic_string<char, std::char_traits<char>, TestAllocator<char>>();
  ASSERT_TRUE(t.c_str() == std::string("\xc3\xa4/\xe2\x82\xac"));
#endif
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").generic_wstring() ==
              std::wstring(L"ä/€"));
#if defined(__cpp_lib_char8_t) && !defined(TURBO_FILESYSTEM_ENFORCE_CPP17_API)
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").generic_u8string() ==
              std::u8string(u8"\xc3\xa4/\xe2\x82\xac"));
#else
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").generic_u8string() ==
              std::string("\xc3\xa4/\xe2\x82\xac"));
#endif
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").generic_u16string() ==
              std::u16string(u"\u00E4/\u20AC"));
  ASSERT_TRUE(fs::u8path("\xc3\xa4/\xe2\x82\xac").generic_u32string() ==
              std::u32string(U"\U000000E4/\U000020AC"));
#endif
}

TEST(FilesystemPath, compare) {
  ASSERT_TRUE(fs::path("/foo/b").compare("/foo/a") > 0);
  ASSERT_TRUE(fs::path("/foo/b").compare("/foo/b") == 0);
  ASSERT_TRUE(fs::path("/foo/b").compare("/foo/c") < 0);

  ASSERT_TRUE(fs::path("/foo/b").compare(std::string("/foo/a")) > 0);
  ASSERT_TRUE(fs::path("/foo/b").compare(std::string("/foo/b")) == 0);
  ASSERT_TRUE(fs::path("/foo/b").compare(std::string("/foo/c")) < 0);

  ASSERT_TRUE(fs::path("/foo/b").compare(fs::path("/foo/a")) > 0);
  ASSERT_TRUE(fs::path("/foo/b").compare(fs::path("/foo/b")) == 0);
  ASSERT_TRUE(fs::path("/foo/b").compare(fs::path("/foo/c")) < 0);

#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("c:\\a\\b").compare("C:\\a\\b") == 0);
  ASSERT_TRUE(fs::path("c:\\a\\b").compare("d:\\a\\b") != 0);
  ASSERT_TRUE(fs::path("c:\\a\\b").compare("C:\\A\\b") != 0);
#endif

#ifdef LWG_2936_BEHAVIOUR
  ASSERT_TRUE(fs::path("/a/b/").compare("/a/b/c") < 0);
  ASSERT_TRUE(fs::path("/a/b/").compare("a/c") > 0);
#endif // LWG_2936_BEHAVIOUR
}

TEST(FilesystemPath, decompose) {
  // root_name()
  ASSERT_TRUE(fs::path("").root_name() == "");
  ASSERT_TRUE(fs::path(".").root_name() == "");
  ASSERT_TRUE(fs::path("..").root_name() == "");
  ASSERT_TRUE(fs::path("foo").root_name() == "");
  ASSERT_TRUE(fs::path("/").root_name() == "");
  ASSERT_TRUE(fs::path("/foo").root_name() == "");
  ASSERT_TRUE(fs::path("foo/").root_name() == "");
  ASSERT_TRUE(fs::path("/foo/").root_name() == "");
  ASSERT_TRUE(fs::path("foo/bar").root_name() == "");
  ASSERT_TRUE(fs::path("/foo/bar").root_name() == "");
  ASSERT_TRUE(fs::path("///foo/bar").root_name() == "");
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("C:/foo").root_name() == "C:");
  ASSERT_TRUE(fs::path("C:\\foo").root_name() == "C:");
  ASSERT_TRUE(fs::path("C:foo").root_name() == "C:");
#endif

  // root_directory()
  ASSERT_TRUE(fs::path("").root_directory() == "");
  ASSERT_TRUE(fs::path(".").root_directory() == "");
  ASSERT_TRUE(fs::path("..").root_directory() == "");
  ASSERT_TRUE(fs::path("foo").root_directory() == "");
  ASSERT_TRUE(fs::path("/").root_directory() == "/");
  ASSERT_TRUE(fs::path("/foo").root_directory() == "/");
  ASSERT_TRUE(fs::path("foo/").root_directory() == "");
  ASSERT_TRUE(fs::path("/foo/").root_directory() == "/");
  ASSERT_TRUE(fs::path("foo/bar").root_directory() == "");
  ASSERT_TRUE(fs::path("/foo/bar").root_directory() == "/");
  ASSERT_TRUE(fs::path("///foo/bar").root_directory() == "/");
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("C:/foo").root_directory() == "/");
  ASSERT_TRUE(fs::path("C:\\foo").root_directory() == "/");
  ASSERT_TRUE(fs::path("C:foo").root_directory() == "");
#endif

  // root_path()
  ASSERT_TRUE(fs::path("").root_path() == "");
  ASSERT_TRUE(fs::path(".").root_path() == "");
  ASSERT_TRUE(fs::path("..").root_path() == "");
  ASSERT_TRUE(fs::path("foo").root_path() == "");
  ASSERT_TRUE(fs::path("/").root_path() == "/");
  ASSERT_TRUE(fs::path("/foo").root_path() == "/");
  ASSERT_TRUE(fs::path("foo/").root_path() == "");
  ASSERT_TRUE(fs::path("/foo/").root_path() == "/");
  ASSERT_TRUE(fs::path("foo/bar").root_path() == "");
  ASSERT_TRUE(fs::path("/foo/bar").root_path() == "/");
  ASSERT_TRUE(fs::path("///foo/bar").root_path() == "/");
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("C:/foo").root_path() == "C:/");
  ASSERT_TRUE(fs::path("C:\\foo").root_path() == "C:/");
  ASSERT_TRUE(fs::path("C:foo").root_path() == "C:");
#endif

  // relative_path()
  ASSERT_TRUE(fs::path("").relative_path() == "");
  ASSERT_TRUE(fs::path(".").relative_path() == ".");
  ASSERT_TRUE(fs::path("..").relative_path() == "..");
  ASSERT_TRUE(fs::path("foo").relative_path() == "foo");
  ASSERT_TRUE(fs::path("/").relative_path() == "");
  ASSERT_TRUE(fs::path("/foo").relative_path() == "foo");
  ASSERT_TRUE(fs::path("foo/").relative_path() == "foo/");
  ASSERT_TRUE(fs::path("/foo/").relative_path() == "foo/");
  ASSERT_TRUE(fs::path("foo/bar").relative_path() == "foo/bar");
  ASSERT_TRUE(fs::path("/foo/bar").relative_path() == "foo/bar");
  ASSERT_TRUE(fs::path("///foo/bar").relative_path() == "foo/bar");
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("C:/foo").relative_path() == "foo");
  ASSERT_TRUE(fs::path("C:\\foo").relative_path() == "foo");
  ASSERT_TRUE(fs::path("C:foo").relative_path() == "foo");
#endif

  // parent_path()
  ASSERT_TRUE(fs::path("").parent_path() == "");
  ASSERT_TRUE(fs::path(".").parent_path() == "");
  ASSERT_TRUE(fs::path("..").parent_path() ==
              ""); // unintuitive but as defined in the standard
  ASSERT_TRUE(fs::path("foo").parent_path() == "");
  ASSERT_TRUE(fs::path("/").parent_path() == "/");
  ASSERT_TRUE(fs::path("/foo").parent_path() == "/");
  ASSERT_TRUE(fs::path("foo/").parent_path() == "foo");
  ASSERT_TRUE(fs::path("/foo/").parent_path() == "/foo");
  ASSERT_TRUE(fs::path("foo/bar").parent_path() == "foo");
  ASSERT_TRUE(fs::path("/foo/bar").parent_path() == "/foo");
  ASSERT_TRUE(fs::path("///foo/bar").parent_path() == "/foo");
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("C:/foo").parent_path() == "C:/");
  ASSERT_TRUE(fs::path("C:\\foo").parent_path() == "C:/");
  ASSERT_TRUE(fs::path("C:foo").parent_path() == "C:");
#endif

  // filename()
  ASSERT_TRUE(fs::path("").filename() == "");
  ASSERT_TRUE(fs::path(".").filename() == ".");
  ASSERT_TRUE(fs::path("..").filename() == "..");
  ASSERT_TRUE(fs::path("foo").filename() == "foo");
  ASSERT_TRUE(fs::path("/").filename() == "");
  ASSERT_TRUE(fs::path("/foo").filename() == "foo");
  ASSERT_TRUE(fs::path("foo/").filename() == "");
  ASSERT_TRUE(fs::path("/foo/").filename() == "");
  ASSERT_TRUE(fs::path("foo/bar").filename() == "bar");
  ASSERT_TRUE(fs::path("/foo/bar").filename() == "bar");
  ASSERT_TRUE(fs::path("///foo/bar").filename() == "bar");
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("C:/foo").filename() == "foo");
  ASSERT_TRUE(fs::path("C:\\foo").filename() == "foo");
  ASSERT_TRUE(fs::path("C:foo").filename() == "foo");
#endif

  // stem()
  ASSERT_TRUE(fs::path("/foo/bar.txt").stem() == "bar");
  {
    fs::path p = "foo.bar.baz.tar";
    ASSERT_TRUE(p.extension() == ".tar");
    p = p.stem();
    ASSERT_TRUE(p.extension() == ".baz");
    p = p.stem();
    ASSERT_TRUE(p.extension() == ".bar");
    p = p.stem();
    ASSERT_TRUE(p == "foo");
  }
  ASSERT_TRUE(fs::path("/foo/.profile").stem() == ".profile");
  ASSERT_TRUE(fs::path(".bar").stem() == ".bar");
  ASSERT_TRUE(fs::path("..bar").stem() == ".");

  // extension()
  ASSERT_TRUE(fs::path("/foo/bar.txt").extension() == ".txt");
  ASSERT_TRUE(fs::path("/foo/bar").extension() == "");
  ASSERT_TRUE(fs::path("/foo/.profile").extension() == "");
  ASSERT_TRUE(fs::path(".bar").extension() == "");
  ASSERT_TRUE(fs::path("..bar").extension() == ".bar");

  if (has_host_root_name_support()) {
    // //host-based root-names
    ASSERT_TRUE(fs::path("//host").root_name() == "//host");
    ASSERT_TRUE(fs::path("//host/foo").root_name() == "//host");
    ASSERT_TRUE(fs::path("//host").root_directory() == "");
    ASSERT_TRUE(fs::path("//host/foo").root_directory() == "/");
    ASSERT_TRUE(fs::path("//host").root_path() == "//host");
    ASSERT_TRUE(fs::path("//host/foo").root_path() == "//host/");
    ASSERT_TRUE(fs::path("//host").relative_path() == "");
    ASSERT_TRUE(fs::path("//host/foo").relative_path() == "foo");
    ASSERT_TRUE(fs::path("//host").parent_path() == "//host");
    ASSERT_TRUE(fs::path("//host/foo").parent_path() == "//host/");
    ASSERT_TRUE(fs::path("//host").filename() == "");
    ASSERT_TRUE(fs::path("//host/foo").filename() == "foo");
  }
}

TEST(FilesystemPath, query) {
  // empty
  ASSERT_TRUE(fs::path("").empty());
  ASSERT_TRUE(!fs::path("foo").empty());

  // has_root_path()
  ASSERT_TRUE(!fs::path("foo").has_root_path());
  ASSERT_TRUE(!fs::path("foo/bar").has_root_path());
  ASSERT_TRUE(fs::path("/foo").has_root_path());
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("C:foo").has_root_path());
  ASSERT_TRUE(fs::path("C:/foo").has_root_path());
#endif

  // has_root_name()
  ASSERT_TRUE(!fs::path("foo").has_root_name());
  ASSERT_TRUE(!fs::path("foo/bar").has_root_name());
  ASSERT_TRUE(!fs::path("/foo").has_root_name());
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("C:foo").has_root_name());
  ASSERT_TRUE(fs::path("C:/foo").has_root_name());
#endif

  // has_root_directory()
  ASSERT_TRUE(!fs::path("foo").has_root_directory());
  ASSERT_TRUE(!fs::path("foo/bar").has_root_directory());
  ASSERT_TRUE(fs::path("/foo").has_root_directory());
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(!fs::path("C:foo").has_root_directory());
  ASSERT_TRUE(fs::path("C:/foo").has_root_directory());
#endif

  // has_relative_path()
  ASSERT_TRUE(!fs::path("").has_relative_path());
  ASSERT_TRUE(!fs::path("/").has_relative_path());
  ASSERT_TRUE(fs::path("/foo").has_relative_path());

  // has_parent_path()
  ASSERT_TRUE(!fs::path("").has_parent_path());
  ASSERT_TRUE(!fs::path(".").has_parent_path());
  ASSERT_TRUE(
      !fs::path("..")
           .has_parent_path()); // unintuitive but as defined in the standard
  ASSERT_TRUE(!fs::path("foo").has_parent_path());
  ASSERT_TRUE(fs::path("/").has_parent_path());
  ASSERT_TRUE(fs::path("/foo").has_parent_path());
  ASSERT_TRUE(fs::path("foo/").has_parent_path());
  ASSERT_TRUE(fs::path("/foo/").has_parent_path());

  // has_filename()
  ASSERT_TRUE(fs::path("foo").has_filename());
  ASSERT_TRUE(fs::path("foo/bar").has_filename());
  ASSERT_TRUE(!fs::path("/foo/bar/").has_filename());

  // has_stem()
  ASSERT_TRUE(fs::path("foo").has_stem());
  ASSERT_TRUE(fs::path("foo.bar").has_stem());
  ASSERT_TRUE(fs::path(".profile").has_stem());
  ASSERT_TRUE(!fs::path("/foo/").has_stem());

  // has_extension()
  ASSERT_TRUE(!fs::path("foo").has_extension());
  ASSERT_TRUE(fs::path("foo.bar").has_extension());
  ASSERT_TRUE(!fs::path(".profile").has_extension());

  // is_absolute()
  ASSERT_TRUE(!fs::path("foo/bar").is_absolute());
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(!fs::path("/foo").is_absolute());
  ASSERT_TRUE(!fs::path("c:foo").is_absolute());
  ASSERT_TRUE(fs::path("c:/foo").is_absolute());
#else
  ASSERT_TRUE(fs::path("/foo").is_absolute());
#endif

  // is_relative()
  ASSERT_TRUE(fs::path("foo/bar").is_relative());
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("/foo").is_relative());
  ASSERT_TRUE(fs::path("c:foo").is_relative());
  ASSERT_TRUE(!fs::path("c:/foo").is_relative());
#else
  ASSERT_TRUE(!fs::path("/foo").is_relative());
#endif

  if (has_host_root_name_support()) {
    ASSERT_TRUE(fs::path("//host").has_root_name());
    ASSERT_TRUE(fs::path("//host/foo").has_root_name());
    ASSERT_TRUE(fs::path("//host").has_root_path());
    ASSERT_TRUE(fs::path("//host/foo").has_root_path());
    ASSERT_TRUE(!fs::path("//host").has_root_directory());
    ASSERT_TRUE(fs::path("//host/foo").has_root_directory());
    ASSERT_TRUE(!fs::path("//host").has_relative_path());
    ASSERT_TRUE(fs::path("//host/foo").has_relative_path());
    ASSERT_TRUE(fs::path("//host/foo").is_absolute());
    ASSERT_TRUE(!fs::path("//host/foo").is_relative());
  }
}

TEST(FilesystemPath, fs_path_gen) {
  // lexically_normal()
  ASSERT_TRUE(fs::path("foo/./bar/..").lexically_normal() == "foo/");
  ASSERT_TRUE(fs::path("foo/.///bar/../").lexically_normal() == "foo/");
  ASSERT_TRUE(fs::path("/foo/../..").lexically_normal() == "/");
  ASSERT_TRUE(fs::path("foo/..").lexically_normal() == ".");
  ASSERT_TRUE(fs::path("ab/cd/ef/../../qw").lexically_normal() == "ab/qw");
  ASSERT_TRUE(fs::path("a/b/../../../c").lexically_normal() == "../c");
  ASSERT_TRUE(fs::path("../").lexically_normal() == "..");
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("\\/\\///\\/").lexically_normal() == "/");
  ASSERT_TRUE(fs::path("a/b/..\\//..///\\/../c\\\\/").lexically_normal() ==
              "../c/");
  ASSERT_TRUE(fs::path("..a/b/..\\//..///\\/../c\\\\/").lexically_normal() ==
              "../c/");
  ASSERT_TRUE(fs::path("..\\").lexically_normal() == "..");
#endif

  // lexically_relative()
  ASSERT_TRUE(fs::path("/a/d").lexically_relative("/a/b/c") == "../../d");
  ASSERT_TRUE(fs::path("/a/b/c").lexically_relative("/a/d") == "../b/c");
  ASSERT_TRUE(fs::path("a/b/c").lexically_relative("a") == "b/c");
  ASSERT_TRUE(fs::path("a/b/c").lexically_relative("a/b/c/x/y") == "../..");
  ASSERT_TRUE(fs::path("a/b/c").lexically_relative("a/b/c") == ".");
  ASSERT_TRUE(fs::path("a/b").lexically_relative("c/d") == "../../a/b");
  ASSERT_TRUE(fs::path("a/b").lexically_relative("a/") == "b");
  if (has_host_root_name_support()) {
    ASSERT_TRUE(fs::path("//host1/foo").lexically_relative("//host2.bar") ==
                "");
  }
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("c:/foo").lexically_relative("/bar") == "");
  ASSERT_TRUE(fs::path("c:foo").lexically_relative("c:/bar") == "");
  ASSERT_TRUE(fs::path("foo").lexically_relative("/bar") == "");
  ASSERT_TRUE(fs::path("c:/foo/bar.txt").lexically_relative("c:/foo/") ==
              "bar.txt");
  ASSERT_TRUE(fs::path("c:/foo/bar.txt").lexically_relative("C:/foo/") ==
              "bar.txt");
#else
  ASSERT_TRUE(fs::path("/foo").lexically_relative("bar") == "");
  ASSERT_TRUE(fs::path("foo").lexically_relative("/bar") == "");
#endif

  // lexically_proximate()
  ASSERT_TRUE(fs::path("/a/d").lexically_proximate("/a/b/c") == "../../d");
  if (has_host_root_name_support()) {
    ASSERT_TRUE(fs::path("//host1/a/d").lexically_proximate("//host2/a/b/c") ==
                "//host1/a/d");
  }
  ASSERT_TRUE(fs::path("a/d").lexically_proximate("/a/b/c") == "a/d");
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE(fs::path("c:/a/d").lexically_proximate("c:/a/b/c") == "../../d");
  ASSERT_TRUE(fs::path("c:/a/d").lexically_proximate("d:/a/b/c") == "c:/a/d");
  ASSERT_TRUE(fs::path("c:/foo").lexically_proximate("/bar") == "c:/foo");
  ASSERT_TRUE(fs::path("c:foo").lexically_proximate("c:/bar") == "c:foo");
  ASSERT_TRUE(fs::path("foo").lexically_proximate("/bar") == "foo");
#else
  ASSERT_TRUE(fs::path("/foo").lexically_proximate("bar") == "/foo");
  ASSERT_TRUE(fs::path("foo").lexically_proximate("/bar") == "foo");
#endif
}

static std::string iterateResult(const fs::path &path) {
  std::ostringstream result;
  for (fs::path::const_iterator i = path.begin(); i != path.end(); ++i) {
    if (i != path.begin()) {
      result << ",";
    }
    result << i->generic_string();
  }
  return result.str();
}

static std::string reverseIterateResult(const fs::path &path) {
  std::ostringstream result;
  fs::path::const_iterator iter = path.end();
  bool first = true;
  if (iter != path.begin()) {
    do {
      --iter;
      if (!first) {
        result << ",";
      }
      first = false;
      result << iter->generic_string();
    } while (iter != path.begin());
  }
  return result.str();
}

TEST(FilesystemPath, itr) {
  ASSERT_TRUE(iterateResult(fs::path()).empty());
  ASSERT_TRUE("." == iterateResult(fs::path(".")));
  ASSERT_TRUE(".." == iterateResult(fs::path("..")));
  ASSERT_TRUE("foo" == iterateResult(fs::path("foo")));
  ASSERT_TRUE("/" == iterateResult(fs::path("/")));
  ASSERT_TRUE("/,foo" == iterateResult(fs::path("/foo")));
  ASSERT_TRUE("foo," == iterateResult(fs::path("foo/")));
  ASSERT_TRUE("/,foo," == iterateResult(fs::path("/foo/")));
  ASSERT_TRUE("foo,bar" == iterateResult(fs::path("foo/bar")));
  ASSERT_TRUE("/,foo,bar" == iterateResult(fs::path("/foo/bar")));
#ifndef USE_STD_FS
  // turbo::filesystem enforces redundant slashes to be reduced to one
  ASSERT_TRUE("/,foo,bar" == iterateResult(fs::path("///foo/bar")));
#else
  // typically std::filesystem keeps them
  ASSERT_TRUE("///,foo,bar" == iterateResult(fs::path("///foo/bar")));
#endif
  ASSERT_TRUE("/,foo,bar," == iterateResult(fs::path("/foo/bar///")));
  ASSERT_TRUE("foo,.,bar,..," == iterateResult(fs::path("foo/.///bar/../")));
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE("C:,/,foo" == iterateResult(fs::path("C:/foo")));
#endif

  ASSERT_TRUE(reverseIterateResult(fs::path()).empty());
  ASSERT_TRUE("." == reverseIterateResult(fs::path(".")));
  ASSERT_TRUE(".." == reverseIterateResult(fs::path("..")));
  ASSERT_TRUE("foo" == reverseIterateResult(fs::path("foo")));
  ASSERT_TRUE("/" == reverseIterateResult(fs::path("/")));
  ASSERT_TRUE("foo,/" == reverseIterateResult(fs::path("/foo")));
  ASSERT_TRUE(",foo" == reverseIterateResult(fs::path("foo/")));
  ASSERT_TRUE(",foo,/" == reverseIterateResult(fs::path("/foo/")));
  ASSERT_TRUE("bar,foo" == reverseIterateResult(fs::path("foo/bar")));
  ASSERT_TRUE("bar,foo,/" == reverseIterateResult(fs::path("/foo/bar")));
#ifndef USE_STD_FS
  // turbo::filesystem enforces redundant slashes to be reduced to one
  ASSERT_TRUE("bar,foo,/" == reverseIterateResult(fs::path("///foo/bar")));
#else
  // typically std::filesystem keeps them
  ASSERT_TRUE("bar,foo,///" == reverseIterateResult(fs::path("///foo/bar")));
#endif
  ASSERT_TRUE(",bar,foo,/" == reverseIterateResult(fs::path("/foo/bar///")));
  ASSERT_TRUE(",..,bar,.,foo" ==
              reverseIterateResult(fs::path("foo/.///bar/../")));
#ifdef TURBO_PLATFORM_WINDOWS
  ASSERT_TRUE("foo,/,C:" == reverseIterateResult(fs::path("C:/foo")));
  ASSERT_TRUE("foo,C:" == reverseIterateResult(fs::path("C:foo")));
#endif
  {
    fs::path p1 = "/foo/bar/test.txt";
    fs::path p2;
    for (auto pe : p1) {
      p2 /= pe;
    }
    ASSERT_TRUE(p1 == p2);
    ASSERT_TRUE("bar" == *(--fs::path("/foo/bar").end()));
    auto p = fs::path("/foo/bar");
    auto pi = p.end();
    pi--;
    ASSERT_TRUE("bar" == *pi);
  }

  if (has_host_root_name_support()) {
    ASSERT_TRUE("foo" == *(--fs::path("//host/foo").end()));
    auto p = fs::path("//host/foo");
    auto pi = p.end();
    pi--;
    ASSERT_TRUE("foo" == *pi);
    ASSERT_TRUE("//host" == iterateResult(fs::path("//host")));
    ASSERT_TRUE("//host,/,foo" == iterateResult(fs::path("//host/foo")));
    ASSERT_TRUE("//host" == reverseIterateResult(fs::path("//host")));
    ASSERT_TRUE("foo,/,//host" == reverseIterateResult(fs::path("//host/foo")));
    {
      fs::path p1 = "//host/foo/bar/test.txt";
      fs::path p2;
      for (auto pe : p1) {
        p2 /= pe;
      }
      ASSERT_TRUE(p1 == p2);
    }
  }
}

TEST(FilesystemPath, nonmember) {
  fs::path p1("foo/bar");
  fs::path p2("some/other");
  fs::swap(p1, p2);
  ASSERT_TRUE(p1 == "some/other");
  ASSERT_TRUE(p2 == "foo/bar");
  ASSERT_TRUE(hash_value(p1));
  ASSERT_TRUE(p2 < p1);
  ASSERT_TRUE(p2 <= p1);
  ASSERT_TRUE(p1 <= p1);
  ASSERT_TRUE(!(p1 < p2));
  ASSERT_TRUE(!(p1 <= p2));
  ASSERT_TRUE(p1 > p2);
  ASSERT_TRUE(p1 >= p2);
  ASSERT_TRUE(p1 >= p1);
  ASSERT_TRUE(!(p2 > p1));
  ASSERT_TRUE(!(p2 >= p1));
  ASSERT_TRUE(p1 != p2);
  ASSERT_TRUE(p1 / p2 == "some/other/foo/bar");
}

TEST(FilesystemPath, io) {
  {
    std::ostringstream os;
    os << fs::path("/root/foo bar");
#ifdef TURBO_PLATFORM_WINDOWS
    ASSERT_TRUE(os.str() == "\"\\\\root\\\\foo bar\"");
#else
    ASSERT_TRUE(os.str() == "\"/root/foo bar\"");
#endif
  }
  {
    std::ostringstream os;
    os << fs::path("/root/foo\"bar");
#ifdef TURBO_PLATFORM_WINDOWS
    ASSERT_TRUE(os.str() == "\"\\\\root\\\\foo\\\"bar\"");
#else
    ASSERT_TRUE(os.str() == "\"/root/foo\\\"bar\"");
#endif
  }

  {
    std::istringstream is("\"/root/foo bar\"");
    fs::path p;
    is >> p;
    ASSERT_TRUE(p == fs::path("/root/foo bar"));
    ASSERT_TRUE((is.flags() & std::ios_base::skipws) == std::ios_base::skipws);
  }
  {
    std::istringstream is("\"/root/foo bar\"");
    is >> std::noskipws;
    fs::path p;
    is >> p;
    ASSERT_TRUE(p == fs::path("/root/foo bar"));
    ASSERT_TRUE((is.flags() & std::ios_base::skipws) != std::ios_base::skipws);
  }
  {
    std::istringstream is("\"/root/foo\\\"bar\"");
    fs::path p;
    is >> p;
    ASSERT_TRUE(p == fs::path("/root/foo\"bar"));
  }
  {
    std::istringstream is("/root/foo");
    fs::path p;
    is >> p;
    ASSERT_TRUE(p == fs::path("/root/foo"));
  }
}

TEST(FilesystemPath, factory) {
  ASSERT_TRUE(fs::u8path("foo/bar") == fs::path("foo/bar"));
  ASSERT_TRUE(fs::u8path("foo/bar") == fs::path("foo/bar"));
  std::string str("/foo/bar/test.txt");
  ASSERT_TRUE(fs::u8path(str.begin(), str.end()) == str);
}

TEST(FilesystemPath, filesystem_error) {
  std::error_code ec(1, std::system_category());
  fs::filesystem_error fse("None", std::error_code());
  fse = fs::filesystem_error("Some error", ec);
  ASSERT_TRUE(fse.code().value() == 1);
  ASSERT_TRUE(!std::string(fse.what()).empty());
  ASSERT_TRUE(fse.path1().empty());
  ASSERT_TRUE(fse.path2().empty());
  fse = fs::filesystem_error("Some error", fs::path("foo/bar"), ec);
  ASSERT_TRUE(!std::string(fse.what()).empty());
  ASSERT_TRUE(fse.path1() == "foo/bar");
  ASSERT_TRUE(fse.path2().empty());
  fse = fs::filesystem_error("Some error", fs::path("foo/bar"),
                             fs::path("some/other"), ec);
  ASSERT_TRUE(!std::string(fse.what()).empty());
  ASSERT_TRUE(fse.path1() == "foo/bar");
  ASSERT_TRUE(fse.path2() == "some/other");
}

constexpr fs::perms constExprOwnerAll() {
  return fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec;
}

TEST(FilesystemPath, fs_enum) {
  static_assert(constExprOwnerAll() == fs::perms::owner_all,
                "constexpr didn't result in owner_all");
  ASSERT_TRUE((fs::perms::owner_read | fs::perms::owner_write |
               fs::perms::owner_exec) == fs::perms::owner_all);
  ASSERT_TRUE((fs::perms::group_read | fs::perms::group_write |
               fs::perms::group_exec) == fs::perms::group_all);
  ASSERT_TRUE((fs::perms::others_read | fs::perms::others_write |
               fs::perms::others_exec) == fs::perms::others_all);
  ASSERT_TRUE((fs::perms::owner_all | fs::perms::group_all |
               fs::perms::others_all) == fs::perms::all);
  ASSERT_TRUE((fs::perms::all | fs::perms::set_uid | fs::perms::set_gid |
               fs::perms::sticky_bit) == fs::perms::mask);
}

TEST(FilesystemPath, file_status) {
  {
    fs::file_status fs;
    ASSERT_TRUE(fs.type() == fs::file_type::none);
    ASSERT_TRUE(fs.permissions() == fs::perms::unknown);
  }
  {
    fs::file_status fs{fs::file_type::regular};
    ASSERT_TRUE(fs.type() == fs::file_type::regular);
    ASSERT_TRUE(fs.permissions() == fs::perms::unknown);
  }
  {
    fs::file_status fs{fs::file_type::directory, fs::perms::owner_read |
                                                     fs::perms::owner_write |
                                                     fs::perms::owner_exec};
    ASSERT_TRUE(fs.type() == fs::file_type::directory);
    ASSERT_TRUE(fs.permissions() == fs::perms::owner_all);
    fs.type(fs::file_type::block);
    ASSERT_TRUE(fs.type() == fs::file_type::block);
    fs.type(fs::file_type::character);
    ASSERT_TRUE(fs.type() == fs::file_type::character);
    fs.type(fs::file_type::fifo);
    ASSERT_TRUE(fs.type() == fs::file_type::fifo);
    fs.type(fs::file_type::symlink);
    ASSERT_TRUE(fs.type() == fs::file_type::symlink);
    fs.type(fs::file_type::socket);
    ASSERT_TRUE(fs.type() == fs::file_type::socket);
    fs.permissions(fs.permissions() | fs::perms::group_all |
                   fs::perms::others_all);
    ASSERT_TRUE(fs.permissions() == fs::perms::all);
  }
  {
    fs::file_status fst(fs::file_type::regular);
    fs::file_status fs(std::move(fst));
    ASSERT_TRUE(fs.type() == fs::file_type::regular);
    ASSERT_TRUE(fs.permissions() == fs::perms::unknown);
  }
#if !defined(USE_STD_FS) || defined(TURBO_FILESYSTEM_RUNNING_CPP20)
  {
    fs::file_status fs1{fs::file_type::regular, fs::perms::owner_read |
                                                    fs::perms::owner_write |
                                                    fs::perms::owner_exec};
    fs::file_status fs2{fs::file_type::regular, fs::perms::owner_read |
                                                    fs::perms::owner_write |
                                                    fs::perms::owner_exec};
    fs::file_status fs3{fs::file_type::directory, fs::perms::owner_read |
                                                      fs::perms::owner_write |
                                                      fs::perms::owner_exec};
    fs::file_status fs4{fs::file_type::regular,
                        fs::perms::owner_read | fs::perms::owner_write};
    ASSERT_TRUE(fs1 == fs2);
    ASSERT_FALSE(fs1 == fs3);
    ASSERT_FALSE(fs1 == fs4);
  }
#endif
}

TEST(FilesystemDir, dir_entry) {
  TemporaryDirectory t;
  std::error_code ec;
  auto de = fs::directory_entry(t.path());
  ASSERT_TRUE(de.path() == t.path());
  ASSERT_TRUE((fs::path)de == t.path());
  ASSERT_TRUE(de.exists());
  ASSERT_TRUE(!de.is_block_file());
  ASSERT_TRUE(!de.is_character_file());
  ASSERT_TRUE(de.is_directory());
  ASSERT_TRUE(!de.is_fifo());
  ASSERT_TRUE(!de.is_other());
  ASSERT_TRUE(!de.is_regular_file());
  ASSERT_TRUE(!de.is_socket());
  ASSERT_TRUE(!de.is_symlink());
  ASSERT_TRUE(de.status().type() == fs::file_type::directory);
  ec.clear();
  ASSERT_TRUE(de.status(ec).type() == fs::file_type::directory);
  ASSERT_TRUE(!ec);
  ASSERT_NO_THROW(de.refresh());
  fs::directory_entry none;
  ASSERT_THROW(none.refresh(), fs::filesystem_error);
  ec.clear();
  ASSERT_NO_THROW(none.refresh(ec));
  ASSERT_TRUE(ec);
  ASSERT_THROW(de.assign(""), fs::filesystem_error);
  ec.clear();
  ASSERT_NO_THROW(de.assign("", ec));
  ASSERT_TRUE(ec);
  generateFile(t.path() / "foo", 1234);
  auto now = fs::file_time_type::clock::now();
  ASSERT_NO_THROW(de.assign(t.path() / "foo"));
  ASSERT_NO_THROW(de.assign(t.path() / "foo", ec));
  ASSERT_TRUE(!ec);
  de = fs::directory_entry(t.path() / "foo");
  ASSERT_TRUE(de.path() == t.path() / "foo");
  ASSERT_TRUE(de.exists());
  ASSERT_TRUE(de.exists(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(!de.is_block_file());
  ASSERT_TRUE(!de.is_block_file(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(!de.is_character_file());
  ASSERT_TRUE(!de.is_character_file(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(!de.is_directory());
  ASSERT_TRUE(!de.is_directory(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(!de.is_fifo());
  ASSERT_TRUE(!de.is_fifo(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(!de.is_other());
  ASSERT_TRUE(!de.is_other(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(de.is_regular_file());
  ASSERT_TRUE(de.is_regular_file(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(!de.is_socket());
  ASSERT_TRUE(!de.is_socket(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(!de.is_symlink());
  ASSERT_TRUE(!de.is_symlink(ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(de.file_size() == 1234);
  ASSERT_TRUE(de.file_size(ec) == 1234);
  ASSERT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(
                           de.last_write_time() - now)
                           .count()) < 3);
  ec.clear();
  ASSERT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(
                           de.last_write_time(ec) - now)
                           .count()) < 3);
  ASSERT_TRUE(!ec);
#ifndef TURBO_PLATFORM_WEB
  ASSERT_TRUE(de.hard_link_count() == 1);
  ASSERT_TRUE(de.hard_link_count(ec) == 1);
  ASSERT_TRUE(!ec);
#endif
  ASSERT_THROW(de.replace_filename("bar"), fs::filesystem_error);
  ASSERT_NO_THROW(de.replace_filename("foo"));
  ec.clear();
  ASSERT_NO_THROW(de.replace_filename("bar", ec));
  ASSERT_TRUE(ec);
  auto de2none = fs::directory_entry();
  ec.clear();
#ifndef TURBO_PLATFORM_WEB
  ASSERT_TRUE(de2none.hard_link_count(ec) == static_cast<uintmax_t>(-1));
  ASSERT_THROW(de2none.hard_link_count(), fs::filesystem_error);
  ASSERT_TRUE(ec);
#endif
  ec.clear();
  ASSERT_NO_THROW(de2none.last_write_time(ec));
  ASSERT_THROW(de2none.last_write_time(), fs::filesystem_error);
  ASSERT_TRUE(ec);
  ec.clear();
  ASSERT_THROW(de2none.file_size(), fs::filesystem_error);
  ASSERT_TRUE(de2none.file_size(ec) == static_cast<uintmax_t>(-1));
  ASSERT_TRUE(ec);
  ec.clear();
  ASSERT_TRUE(de2none.status().type() == fs::file_type::not_found);
  ASSERT_TRUE(de2none.status(ec).type() == fs::file_type::not_found);
  ASSERT_TRUE(ec);
  generateFile(t.path() / "a");
  generateFile(t.path() / "b");
  auto d1 = fs::directory_entry(t.path() / "a");
  auto d2 = fs::directory_entry(t.path() / "b");
  ASSERT_TRUE(d1 < d2);
  ASSERT_TRUE(!(d2 < d1));
  ASSERT_TRUE(d1 <= d2);
  ASSERT_TRUE(!(d2 <= d1));
  ASSERT_TRUE(d2 > d1);
  ASSERT_TRUE(!(d1 > d2));
  ASSERT_TRUE(d2 >= d1);
  ASSERT_TRUE(!(d1 >= d2));
  ASSERT_TRUE(d1 != d2);
  ASSERT_TRUE(!(d2 != d2));
  ASSERT_TRUE(d1 == d1);
  ASSERT_TRUE(!(d1 == d2));
}

TEST(FilesystemDir, directory_iterator) {
  {
    TemporaryDirectory t;
    ASSERT_TRUE(fs::directory_iterator(t.path()) == fs::directory_iterator());
    generateFile(t.path() / "test", 1234);
    ASSERT_TRUE(fs::directory_iterator(t.path()) != fs::directory_iterator());
    auto iter = fs::directory_iterator(t.path());
    fs::directory_iterator iter2(iter);
    fs::directory_iterator iter3, iter4;
    iter3 = iter;
    ASSERT_TRUE(iter->path().filename() == "test");
    ASSERT_TRUE(iter2->path().filename() == "test");
    ASSERT_TRUE(iter3->path().filename() == "test");
    iter4 = std::move(iter3);
    ASSERT_TRUE(iter4->path().filename() == "test");
    ASSERT_TRUE(iter->path() == t.path() / "test");
    ASSERT_TRUE(!iter->is_symlink());
    ASSERT_TRUE(iter->is_regular_file());
    ASSERT_TRUE(!iter->is_directory());
    ASSERT_TRUE(iter->file_size() == 1234);
    ASSERT_TRUE(++iter == fs::directory_iterator());
    ASSERT_THROW(fs::directory_iterator(t.path() / "non-existing"),
                 fs::filesystem_error);
    int cnt = 0;
    for (auto de : fs::directory_iterator(t.path())) {
      ++cnt;
    }
    ASSERT_TRUE(cnt == 1);
  }
  if (is_symlink_creation_supported()) {
    TemporaryDirectory t;
    fs::path td = t.path() / "testdir";
    ASSERT_TRUE(fs::directory_iterator(t.path()) == fs::directory_iterator());
    generateFile(t.path() / "test", 1234);
    fs::create_directory(td);
    ASSERT_NO_THROW(fs::create_symlink(t.path() / "test", td / "testlink"));
    std::error_code ec;
    ASSERT_TRUE(fs::directory_iterator(td) != fs::directory_iterator());
    auto iter = fs::directory_iterator(td);
    ASSERT_TRUE(iter->path().filename() == "testlink");
    ASSERT_TRUE(iter->path() == td / "testlink");
    ASSERT_TRUE(iter->is_symlink());
    ASSERT_TRUE(iter->is_regular_file());
    ASSERT_TRUE(!iter->is_directory());
    ASSERT_TRUE(iter->file_size() == 1234);
    ASSERT_TRUE(++iter == fs::directory_iterator());
  }
  {
    // Issue #8: check if resources are freed when iterator reaches end()
    TemporaryDirectory t(TempOpt::change_path);
    auto p = fs::path("test/");
    fs::create_directory(p);
    auto iter = fs::directory_iterator(p);
    while (iter != fs::directory_iterator()) {
      ++iter;
    }
    ASSERT_TRUE(fs::remove_all(p) == 1);
    ASSERT_NO_THROW(fs::create_directory(p));
  }
}

TEST(FilesystemDir, rec_dir_itr) {
  {
    auto iter = fs::recursive_directory_iterator(".");
    iter.pop();
    ASSERT_TRUE(iter == fs::recursive_directory_iterator());
  }
  {
    TemporaryDirectory t;
    ASSERT_TRUE(fs::recursive_directory_iterator(t.path()) ==
                fs::recursive_directory_iterator());
    generateFile(t.path() / "test", 1234);
    ASSERT_TRUE(fs::recursive_directory_iterator(t.path()) !=
                fs::recursive_directory_iterator());
    auto iter = fs::recursive_directory_iterator(t.path());
    ASSERT_TRUE(iter->path().filename() == "test");
    ASSERT_TRUE(iter->path() == t.path() / "test");
    ASSERT_TRUE(!iter->is_symlink());
    ASSERT_TRUE(iter->is_regular_file());
    ASSERT_TRUE(!iter->is_directory());
    ASSERT_TRUE(iter->file_size() == 1234);
    ASSERT_TRUE(++iter == fs::recursive_directory_iterator());
  }

  {
    TemporaryDirectory t;
    fs::path td = t.path() / "testdir";
    fs::create_directories(td);
    generateFile(td / "test", 1234);
    ASSERT_TRUE(fs::recursive_directory_iterator(t.path()) !=
                fs::recursive_directory_iterator());
    auto iter = fs::recursive_directory_iterator(t.path());

    ASSERT_TRUE(iter->path().filename() == "testdir");
    ASSERT_TRUE(iter->path() == td);
    ASSERT_TRUE(!iter->is_symlink());
    ASSERT_TRUE(!iter->is_regular_file());
    ASSERT_TRUE(iter->is_directory());

    ASSERT_TRUE(++iter != fs::recursive_directory_iterator());

    ASSERT_TRUE(iter->path().filename() == "test");
    ASSERT_TRUE(iter->path() == td / "test");
    ASSERT_TRUE(!iter->is_symlink());
    ASSERT_TRUE(iter->is_regular_file());
    ASSERT_TRUE(!iter->is_directory());
    ASSERT_TRUE(iter->file_size() == 1234);

    ASSERT_TRUE(++iter == fs::recursive_directory_iterator());
  }
  {
    TemporaryDirectory t;
    std::error_code ec;
    ASSERT_TRUE(fs::recursive_directory_iterator(t.path(),
                                                 fs::directory_options::none) ==
                fs::recursive_directory_iterator());
    ASSERT_TRUE(fs::recursive_directory_iterator(
                    t.path(), fs::directory_options::none, ec) ==
                fs::recursive_directory_iterator());
    ASSERT_TRUE(!ec);
    ASSERT_TRUE(fs::recursive_directory_iterator(t.path(), ec) ==
                fs::recursive_directory_iterator());
    ASSERT_TRUE(!ec);
    generateFile(t.path() / "test");
    fs::recursive_directory_iterator rd1(t.path());
    ASSERT_TRUE(fs::recursive_directory_iterator(rd1) !=
                fs::recursive_directory_iterator());
    fs::recursive_directory_iterator rd2(t.path());
    ASSERT_TRUE(fs::recursive_directory_iterator(std::move(rd2)) !=
                fs::recursive_directory_iterator());
    fs::recursive_directory_iterator rd3(
        t.path(), fs::directory_options::skip_permission_denied);
    ASSERT_TRUE(rd3.options() == fs::directory_options::skip_permission_denied);
    fs::recursive_directory_iterator rd4;
    rd4 = std::move(rd3);
    ASSERT_TRUE(rd4 != fs::recursive_directory_iterator());
    ASSERT_NO_THROW(++rd4);
    ASSERT_TRUE(rd4 == fs::recursive_directory_iterator());
    fs::recursive_directory_iterator rd5;
    rd5 = rd4;
  }
  {
    TemporaryDirectory t(TempOpt::change_path);
    generateFile("a");
    fs::create_directory("d1");
    fs::create_directory("d1/d2");
    generateFile("d1/b");
    generateFile("d1/c");
    generateFile("d1/d2/d");
    generateFile("e");
    auto iter = fs::recursive_directory_iterator(".");
    std::multimap<std::string, int> result;
    while (iter != fs::recursive_directory_iterator()) {
      result.insert(
          std::make_pair(iter->path().generic_string(), iter.depth()));
      ++iter;
    }
    std::stringstream os;
    for (auto p : result) {
      os << "[" << p.first << "," << p.second << "],";
    }
    ASSERT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/b,1],[./d1/c,1],[./d1/"
                            "d2,1],[./d1/d2/d,2],[./e,0],");
  }
  {
    TemporaryDirectory t(TempOpt::change_path);
    generateFile("a");
    fs::create_directory("d1");
    fs::create_directory("d1/d2");
    generateFile("d1/b");
    generateFile("d1/c");
    generateFile("d1/d2/d");
    generateFile("e");
    std::multiset<std::string> result;
    for (auto de : fs::recursive_directory_iterator(".")) {
      result.insert(de.path().generic_string());
    }
    std::stringstream os;
    for (auto p : result) {
      os << p << ",";
    }
    ASSERT_TRUE(os.str() == "./a,./d1,./d1/b,./d1/c,./d1/d2,./d1/d2/d,./e,");
  }
  {
    TemporaryDirectory t(TempOpt::change_path);
    generateFile("a");
    fs::create_directory("d1");
    fs::create_directory("d1/d2");
    generateFile("d1/d2/b");
    generateFile("e");
    auto iter = fs::recursive_directory_iterator(".");
    std::multimap<std::string, int> result;
    while (iter != fs::recursive_directory_iterator()) {
      result.insert(
          std::make_pair(iter->path().generic_string(), iter.depth()));
      if (iter->path() == "./d1/d2") {
        iter.disable_recursion_pending();
      }
      ++iter;
    }
    std::stringstream os;
    for (auto p : result) {
      os << "[" << p.first << "," << p.second << "],";
    }
    ASSERT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/d2,1],[./e,0],");
  }
  {
    TemporaryDirectory t(TempOpt::change_path);
    generateFile("a");
    fs::create_directory("d1");
    fs::create_directory("d1/d2");
    generateFile("d1/d2/b");
    generateFile("e");
    auto iter = fs::recursive_directory_iterator(".");
    std::multimap<std::string, int> result;
    while (iter != fs::recursive_directory_iterator()) {
      result.insert(
          std::make_pair(iter->path().generic_string(), iter.depth()));
      if (iter->path() == "./d1/d2") {
        iter.pop();
      } else {
        ++iter;
      }
    }
    std::stringstream os;
    for (auto p : result) {
      os << "[" << p.first << "," << p.second << "],";
    }
    ASSERT_TRUE(os.str() == "[./a,0],[./d1,0],[./d1/d2,1],[./e,0],");
  }
  if (is_symlink_creation_supported()) {
    TemporaryDirectory t(TempOpt::change_path);
    fs::create_directory("d1");
    generateFile("d1/a");
    fs::create_directory("d2");
    generateFile("d2/b");
    fs::create_directory_symlink("../d1", "d2/ds1");
    fs::create_directory_symlink("d3", "d2/ds2");
    std::multiset<std::string> result;
    ASSERT_NO_THROW([&]() {
      for (const auto &de : fs::recursive_directory_iterator(
               "d2", fs::directory_options::follow_directory_symlink)) {
        result.insert(de.path().generic_string());
      }
    }());
    std::stringstream os;
    for (const auto &p : result) {
      os << p << ",";
    }
    ASSERT_TRUE(os.str() == "d2/b,d2/ds1,d2/ds1/a,d2/ds2,");
    os.str("");
    result.clear();
    ASSERT_NO_THROW([&]() {
      for (const auto &de : fs::recursive_directory_iterator("d2")) {
        result.insert(de.path().generic_string());
      }
    }());
    for (const auto &p : result) {
      os << p << ",";
    }
    ASSERT_TRUE(os.str() == "d2/b,d2/ds1,d2/ds2,");
  }
}

TEST(FilesystemDir, op_absolute) {
  ASSERT_TRUE(fs::absolute("") == fs::current_path() / "");
  ASSERT_TRUE(fs::absolute(fs::current_path()) == fs::current_path());
  ASSERT_TRUE(fs::absolute(".") == fs::current_path() / ".");
  ASSERT_TRUE((fs::absolute("..") == fs::current_path().parent_path() ||
               fs::absolute("..") == fs::current_path() / ".."));
  ASSERT_TRUE(fs::absolute("foo") == fs::current_path() / "foo");
  std::error_code ec;
  ASSERT_TRUE(fs::absolute("", ec) == fs::current_path() / "");
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::absolute("foo", ec) == fs::current_path() / "foo");
  ASSERT_TRUE(!ec);
}

TEST(FilesystemDir, op_canonical) {
  ASSERT_THROW(fs::canonical(""), fs::filesystem_error);
  {
    std::error_code ec;
    ASSERT_TRUE(fs::canonical("", ec) == "");
    ASSERT_TRUE(ec);
  }
  ASSERT_TRUE(fs::canonical(fs::current_path()) == fs::current_path());

  ASSERT_TRUE(fs::canonical(".") == fs::current_path());
  ASSERT_TRUE(fs::canonical("..") == fs::current_path().parent_path());
  ASSERT_TRUE(fs::canonical("/") == fs::current_path().root_path());
  ASSERT_THROW(fs::canonical("foo"), fs::filesystem_error);
  {
    std::error_code ec;
    ASSERT_NO_THROW(fs::canonical("foo", ec));
    ASSERT_TRUE(ec);
  }
  {
    TemporaryDirectory t(TempOpt::change_path);
    auto dir = t.path() / "d0";
    fs::create_directories(dir / "d1");
    generateFile(dir / "f0");
    fs::path rel(dir.filename());
    ASSERT_TRUE(fs::canonical(dir) == dir);
    ASSERT_TRUE(fs::canonical(rel) == dir);
    ASSERT_TRUE(fs::canonical(dir / "f0") == dir / "f0");
    ASSERT_TRUE(fs::canonical(rel / "f0") == dir / "f0");
    ASSERT_TRUE(fs::canonical(rel / "./f0") == dir / "f0");
    ASSERT_TRUE(fs::canonical(rel / "d1/../f0") == dir / "f0");
  }

  if (is_symlink_creation_supported()) {
    TemporaryDirectory t(TempOpt::change_path);
    fs::create_directory(t.path() / "dir1");
    generateFile(t.path() / "dir1/test1");
    fs::create_directory(t.path() / "dir2");
    fs::create_directory_symlink(t.path() / "dir1", t.path() / "dir2/dirSym");
    ASSERT_TRUE(fs::canonical(t.path() / "dir2/dirSym/test1") ==
                t.path() / "dir1/test1");
  }
}

TEST(FilesystemDir, op_copy) {
  {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    fs::create_directory("dir1");
    generateFile("dir1/file1");
    generateFile("dir1/file2");
    fs::create_directory("dir1/dir2");
    generateFile("dir1/dir2/file3");
    ASSERT_NO_THROW(fs::copy("dir1", "dir3"));
    ASSERT_TRUE(fs::exists("dir3/file1"));
    ASSERT_TRUE(fs::exists("dir3/file2"));
    ASSERT_TRUE(!fs::exists("dir3/dir2"));
    ASSERT_NO_THROW(fs::copy("dir1", "dir4", fs::copy_options::recursive, ec));
    ASSERT_TRUE(!ec);
    ASSERT_TRUE(fs::exists("dir4/file1"));
    ASSERT_TRUE(fs::exists("dir4/file2"));
    ASSERT_TRUE(fs::exists("dir4/dir2/file3"));
    fs::create_directory("dir5");
    generateFile("dir5/file1");
    ASSERT_THROW(fs::copy("dir1/file1", "dir5/file1"), fs::filesystem_error);
    ASSERT_NO_THROW(
        fs::copy("dir1/file1", "dir5/file1", fs::copy_options::skip_existing));
  }
  if (is_symlink_creation_supported()) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    fs::create_directory("dir1");
    generateFile("dir1/file1");
    generateFile("dir1/file2");
    fs::create_directory("dir1/dir2");
    generateFile("dir1/dir2/file3");
#ifdef TEST_LWG_2682_BEHAVIOUR
    ASSERT_THROW(fs::copy("dir1", "dir3",
                          fs::copy_options::create_symlinks |
                              fs::copy_options::recursive),
                 fs::filesystem_error);
#else
    ASSERT_NO_THROW(fs::copy("dir1", "dir3",
                             fs::copy_options::create_symlinks |
                                 fs::copy_options::recursive));
    ASSERT_TRUE(!ec);
    ASSERT_TRUE(fs::exists("dir3/file1"));
    ASSERT_TRUE(fs::is_symlink("dir3/file1"));
    ASSERT_TRUE(fs::exists("dir3/file2"));
    ASSERT_TRUE(fs::is_symlink("dir3/file2"));
    ASSERT_TRUE(fs::exists("dir3/dir2/file3"));
    ASSERT_TRUE(fs::is_symlink("dir3/dir2/file3"));
#endif
  }
#ifndef TURBO_PLATFORM_WEB
  {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    fs::create_directory("dir1");
    generateFile("dir1/file1");
    generateFile("dir1/file2");
    fs::create_directory("dir1/dir2");
    generateFile("dir1/dir2/file3");
    auto f1hl = fs::hard_link_count("dir1/file1");
    auto f2hl = fs::hard_link_count("dir1/file2");
    auto f3hl = fs::hard_link_count("dir1/dir2/file3");
    ASSERT_NO_THROW(fs::copy(
        "dir1", "dir3",
        fs::copy_options::create_hard_links | fs::copy_options::recursive, ec));
    ASSERT_TRUE(!ec);
    ASSERT_TRUE(fs::exists("dir3/file1"));
    ASSERT_TRUE(fs::hard_link_count("dir1/file1") == f1hl + 1);
    ASSERT_TRUE(fs::exists("dir3/file2"));
    ASSERT_TRUE(fs::hard_link_count("dir1/file2") == f2hl + 1);
    ASSERT_TRUE(fs::exists("dir3/dir2/file3"));
    ASSERT_TRUE(fs::hard_link_count("dir1/dir2/file3") == f3hl + 1);
  }
#endif
}

TEST(Filesystem, copy_file) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo", 100);
  ASSERT_TRUE(!fs::exists("bar"));
  ASSERT_TRUE(fs::copy_file("foo", "bar"));
  ASSERT_TRUE(fs::exists("bar"));
  ASSERT_TRUE(fs::file_size("foo") == fs::file_size("bar"));
  ASSERT_TRUE(fs::copy_file("foo", "bar2", ec));
  ASSERT_TRUE(!ec);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  generateFile("foo2", 200);
  ASSERT_TRUE(fs::copy_file("foo2", "bar", fs::copy_options::update_existing));
  ASSERT_TRUE(fs::file_size("bar") == 200);
  ASSERT_TRUE(!fs::copy_file("foo", "bar", fs::copy_options::update_existing));
  ASSERT_TRUE(fs::file_size("bar") == 200);
  ASSERT_TRUE(
      fs::copy_file("foo", "bar", fs::copy_options::overwrite_existing));
  ASSERT_TRUE(fs::file_size("bar") == 100);
  ASSERT_THROW(fs::copy_file("foobar", "foobar2"), fs::filesystem_error);
  ASSERT_NO_THROW(fs::copy_file("foobar", "foobar2", ec));
  ASSERT_TRUE(ec);
  ASSERT_TRUE(!fs::exists("foobar"));
}

TEST(FilesystemDir, copy_symlink) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo");
  fs::create_directory("dir");
  if (is_symlink_creation_supported()) {
    fs::create_symlink("foo", "sfoo");
    fs::create_directory_symlink("dir", "sdir");
    ASSERT_NO_THROW(fs::copy_symlink("sfoo", "sfooc"));
    ASSERT_TRUE(fs::exists("sfooc"));
    ASSERT_NO_THROW(fs::copy_symlink("sfoo", "sfooc2", ec));
    ASSERT_TRUE(fs::exists("sfooc2"));
    ASSERT_TRUE(!ec);
    ASSERT_NO_THROW(fs::copy_symlink("sdir", "sdirc"));
    ASSERT_TRUE(fs::exists("sdirc"));
    ASSERT_NO_THROW(fs::copy_symlink("sdir", "sdirc2", ec));
    ASSERT_TRUE(fs::exists("sdirc2"));
    ASSERT_TRUE(!ec);
  }
  ASSERT_THROW(fs::copy_symlink("bar", "barc"), fs::filesystem_error);
  ASSERT_NO_THROW(fs::copy_symlink("bar", "barc", ec));
  ASSERT_TRUE(ec);
}

TEST(FilesystemDir, create_directories) {
  TemporaryDirectory t;
  fs::path p = t.path() / "testdir";
  fs::path p2 = p / "nested";
  ASSERT_TRUE(!fs::exists(p));
  ASSERT_TRUE(!fs::exists(p2));
  ASSERT_TRUE(fs::create_directories(p2));
  ASSERT_TRUE(fs::is_directory(p));
  ASSERT_TRUE(fs::is_directory(p2));
  ASSERT_TRUE(!fs::create_directories(p2));
#ifdef TEST_LWG_2935_BEHAVIOUR
  INFO("This test expects LWG #2935 result conformance.");
  p = t.path() / "testfile";
  generateFile(p);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  bool created = false;
  ASSERT_NO_THROW((created = fs::create_directories(p)));
  ASSERT_TRUE(!created);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  std::error_code ec;
  ASSERT_NO_THROW((created = fs::create_directories(p, ec)));
  ASSERT_TRUE(!created);
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  ASSERT_TRUE(!fs::create_directories(p, ec));
#else
  TURBO_LOG(INFO) << "This test expects conformance with P1164R1. (implemented "
                     "by GCC with issue #86910.)";
  p = t.path() / "testfile";
  generateFile(p);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  ASSERT_THROW(fs::create_directories(p), fs::filesystem_error);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  std::error_code ec;
  ASSERT_NO_THROW(fs::create_directories(p, ec));
  ASSERT_TRUE(ec);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  ASSERT_TRUE(!fs::create_directories(p, ec));
#endif
}

TEST(FilesystemDir, create_directory) {
  TemporaryDirectory t;
  fs::path p = t.path() / "testdir";
  ASSERT_TRUE(!fs::exists(p));
  ASSERT_TRUE(fs::create_directory(p));
  ASSERT_TRUE(fs::is_directory(p));
  ASSERT_TRUE(!fs::is_regular_file(p));
  ASSERT_TRUE(fs::create_directory(p / "nested", p));
  ASSERT_TRUE(fs::is_directory(p / "nested"));
  ASSERT_TRUE(!fs::is_regular_file(p / "nested"));
#ifdef TEST_LWG_2935_BEHAVIOUR
  TURBO_LOG(INFO) << "This test expects LWG #2935 result conformance.";
  p = t.path() / "testfile";
  generateFile(p);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  bool created = false;
  ASSERT_NO_THROW((created = fs::create_directory(p)));
  ASSERT_TRUE(!created);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  std::error_code ec;
  ASSERT_NO_THROW((created = fs::create_directory(p, ec)));
  ASSERT_TRUE(!created);
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  ASSERT_TRUE(!fs::create_directories(p, ec));
#else
  TURBO_LOG(INFO) << "This test expects conformance with P1164R1. (implemented "
                     "by GCC with issue #86910.)";
  p = t.path() / "testfile";
  generateFile(p);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  ASSERT_THROW(fs::create_directory(p), fs::filesystem_error);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  std::error_code ec;
  ASSERT_NO_THROW(fs::create_directory(p, ec));
  ASSERT_TRUE(ec);
  ASSERT_TRUE(fs::is_regular_file(p));
  ASSERT_TRUE(!fs::is_directory(p));
  ASSERT_TRUE(!fs::create_directory(p, ec));
#endif
}

TEST(FilesystemDir, create_directory_symlink) {
  if (is_symlink_creation_supported()) {
    TemporaryDirectory t;
    fs::create_directory(t.path() / "dir1");
    generateFile(t.path() / "dir1/test1");
    fs::create_directory(t.path() / "dir2");
    fs::create_directory_symlink(t.path() / "dir1", t.path() / "dir2/dirSym");
    ASSERT_TRUE(fs::exists(t.path() / "dir2/dirSym"));
    ASSERT_TRUE(fs::is_symlink(t.path() / "dir2/dirSym"));
    ASSERT_TRUE(fs::exists(t.path() / "dir2/dirSym/test1"));
    ASSERT_TRUE(fs::is_regular_file(t.path() / "dir2/dirSym/test1"));
    ASSERT_THROW(fs::create_directory_symlink(t.path() / "dir1",
                                              t.path() / "dir2/dirSym"),
                 fs::filesystem_error);
    std::error_code ec;
    ASSERT_NO_THROW(fs::create_directory_symlink(t.path() / "dir1",
                                                 t.path() / "dir2/dirSym", ec));
    ASSERT_TRUE(ec);
  }
}

TEST(FilesystemDir, create_hard_link) {
#ifndef TURBO_PLATFORM_WEB
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo", 1234);
  ASSERT_NO_THROW(fs::create_hard_link("foo", "bar"));
  ASSERT_TRUE(fs::exists("bar"));
  ASSERT_TRUE(!fs::is_symlink("bar"));
  ASSERT_NO_THROW(fs::create_hard_link("foo", "bar2", ec));
  ASSERT_TRUE(fs::exists("bar2"));
  ASSERT_TRUE(!fs::is_symlink("bar2"));
  ASSERT_TRUE(!ec);
  ASSERT_THROW(fs::create_hard_link("nofoo", "bar"), fs::filesystem_error);
  ASSERT_NO_THROW(fs::create_hard_link("nofoo", "bar", ec));
  ASSERT_TRUE(ec);
#endif
}

TEST(FilesystemDir, create_symlink) {
  if (is_symlink_creation_supported()) {
    TemporaryDirectory t;
    fs::create_directory(t.path() / "dir1");
    generateFile(t.path() / "dir1/test1");
    fs::create_directory(t.path() / "dir2");
    fs::create_symlink(t.path() / "dir1/test1", t.path() / "dir2/fileSym");
    ASSERT_TRUE(fs::exists(t.path() / "dir2/fileSym"));
    ASSERT_TRUE(fs::is_symlink(t.path() / "dir2/fileSym"));
    ASSERT_TRUE(fs::exists(t.path() / "dir2/fileSym"));
    ASSERT_TRUE(fs::is_regular_file(t.path() / "dir2/fileSym"));
    ASSERT_THROW(
        fs::create_symlink(t.path() / "dir1", t.path() / "dir2/fileSym"),
        fs::filesystem_error);
    std::error_code ec;
    ASSERT_NO_THROW(
        fs::create_symlink(t.path() / "dir1", t.path() / "dir2/fileSym", ec));
    ASSERT_TRUE(ec);
  }
}

TEST(FilesystemDir, current_path) {
  TemporaryDirectory t;
  std::error_code ec;
  fs::path p1 = fs::current_path();
  ASSERT_NO_THROW(fs::current_path(t.path()));
  ASSERT_TRUE(p1 != fs::current_path());
  ASSERT_NO_THROW(fs::current_path(p1, ec));
  ASSERT_TRUE(!ec);
  ASSERT_THROW(fs::current_path(t.path() / "foo"), fs::filesystem_error);
  ASSERT_TRUE(p1 == fs::current_path());
  ASSERT_NO_THROW(fs::current_path(t.path() / "foo", ec));
  ASSERT_TRUE(ec);
}

TEST(FilesystemDir, equivalent) {
  TemporaryDirectory t(TempOpt::change_path);
  generateFile("foo", 1234);
  ASSERT_TRUE(fs::equivalent(t.path() / "foo", "foo"));
  if (is_symlink_creation_supported()) {
    std::error_code ec(42, std::system_category());
    fs::create_symlink("foo", "foo2");
    ASSERT_TRUE(fs::equivalent("foo", "foo2"));
    ASSERT_TRUE(fs::equivalent("foo", "foo2", ec));
    ASSERT_TRUE(!ec);
  }
#ifdef TEST_LWG_2937_BEHAVIOUR
  TURBO_LOG(INFO) << "This test expects LWG #2937 result conformance.";
  std::error_code ec;
  bool result = false;
  ASSERT_THROW(fs::equivalent("foo", "foo3"), fs::filesystem_error);
  ASSERT_NO_THROW(result = fs::equivalent("foo", "foo3", ec));
  ASSERT_TRUE(!result);
  ASSERT_TRUE(ec);
  ec.clear();
  ASSERT_THROW(fs::equivalent("foo3", "foo"), fs::filesystem_error);
  ASSERT_NO_THROW(result = fs::equivalent("foo3", "foo", ec));
  ASSERT_TRUE(!result);
  ASSERT_TRUE(ec);
  ec.clear();
  ASSERT_THROW(fs::equivalent("foo3", "foo4"), fs::filesystem_error);
  ASSERT_NO_THROW(result = fs::equivalent("foo3", "foo4", ec));
  ASSERT_TRUE(!result);
  ASSERT_TRUE(ec);
#else
  TURBO_LOG(INFO)
      << "This test expects conformance predating LWG #2937 result.";
  std::error_code ec;
  bool result = false;
  ASSERT_NO_THROW(result = fs::equivalent("foo", "foo3"));
  ASSERT_TRUE(!result);
  ASSERT_NO_THROW(result = fs::equivalent("foo", "foo3", ec));
  ASSERT_TRUE(!result);
  ASSERT_TRUE(!ec);
  ec.clear();
  ASSERT_NO_THROW(result = fs::equivalent("foo3", "foo"));
  ASSERT_TRUE(!result);
  ASSERT_NO_THROW(result = fs::equivalent("foo3", "foo", ec));
  ASSERT_TRUE(!result);
  ASSERT_TRUE(!ec);
  ec.clear();
  ASSERT_THROW(result = fs::equivalent("foo4", "foo3"), fs::filesystem_error);
  ASSERT_TRUE(!result);
  ASSERT_NO_THROW(result = fs::equivalent("foo4", "foo3", ec));
  ASSERT_TRUE(!result);
  ASSERT_TRUE(ec);
#endif
}

TEST(Filesystem, exists) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  ASSERT_TRUE(!fs::exists(""));
  ASSERT_TRUE(!fs::exists("foo"));
  ASSERT_TRUE(!fs::exists("foo", ec));
  ASSERT_TRUE(!ec);
  ec = std::error_code(42, std::system_category());
  ASSERT_TRUE(!fs::exists("foo", ec));
#if defined(__cpp_lib_char8_t) && !defined(TURBO_FILESYSTEM_ENFORCE_CPP17_API)
  ASSERT_TRUE(!fs::exists(u8"foo"));
#endif
  ASSERT_TRUE(!ec);
  ec.clear();
  ASSERT_TRUE(fs::exists(t.path()));
  ASSERT_TRUE(fs::exists(t.path(), ec));
  ASSERT_TRUE(!ec);
  ec = std::error_code(42, std::system_category());
  ASSERT_TRUE(fs::exists(t.path(), ec));
  ASSERT_TRUE(!ec);
#if defined(TURBO_PLATFORM_WINDOWS)
  if (::GetFileAttributesW(L"C:\\fs-test") != INVALID_FILE_ATTRIBUTES) {
    ASSERT_TRUE(fs::exists("C:\\fs-test"));
  }
#endif
}

TEST(Filesystem, file_size) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo", 0);
  generateFile("bar", 1234);
  ASSERT_TRUE(fs::file_size("foo") == 0);
  ec = std::error_code(42, std::system_category());
  ASSERT_TRUE(fs::file_size("foo", ec) == 0);
  ASSERT_TRUE(!ec);
  ec.clear();
  ASSERT_TRUE(fs::file_size("bar") == 1234);
  ec = std::error_code(42, std::system_category());
  ASSERT_TRUE(fs::file_size("bar", ec) == 1234);
  ASSERT_TRUE(!ec);
  ec.clear();
  ASSERT_THROW(fs::file_size("foobar"), fs::filesystem_error);
  ASSERT_TRUE(fs::file_size("foobar", ec) == static_cast<uintmax_t>(-1));
  ASSERT_TRUE(ec);
  ec.clear();
}

#ifndef TURBO_PLATFORM_WINDOWS
static uintmax_t getHardlinkCount(const fs::path &p) {
  struct stat st = {};
  auto rc = ::lstat(p.c_str(), &st);
  return rc == 0 ? st.st_nlink : ~0u;
}
#endif

TEST(Filesystem, hard_link_count) {
#ifndef TURBO_PLATFORM_WEB
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
#ifdef TURBO_PLATFORM_WINDOWS
  // windows doesn't implement "."/".." as hardlinks, so it
  // starts with 1 and subdirectories don't change the count
  ASSERT_TRUE(fs::hard_link_count(t.path()) == 1);
  fs::create_directory("dir");
  ASSERT_TRUE(fs::hard_link_count(t.path()) == 1);
#else
  // unix/bsd/linux typically implements "."/".." as hardlinks
  // so an empty dir has 2 (from parent and the ".") and
  // adding a subdirectory adds one due to its ".."
  ASSERT_TRUE(fs::hard_link_count(t.path()) == getHardlinkCount(t.path()));
  fs::create_directory("dir");
  ASSERT_TRUE(fs::hard_link_count(t.path()) == getHardlinkCount(t.path()));
#endif
  generateFile("foo");
  ASSERT_TRUE(fs::hard_link_count(t.path() / "foo") == 1);
  ec = std::error_code(42, std::system_category());
  ASSERT_TRUE(fs::hard_link_count(t.path() / "foo", ec) == 1);
  ASSERT_TRUE(!ec);
  ASSERT_THROW(fs::hard_link_count(t.path() / "bar"), fs::filesystem_error);
  ASSERT_NO_THROW(fs::hard_link_count(t.path() / "bar", ec));
  ASSERT_TRUE(ec);
  ec.clear();
#else
  WARN("Test for unsupportet features are disabled on JS/Wasm target.");
#endif
}

class FileTypeMixFixture {
public:
  FileTypeMixFixture()
      : _t(TempOpt::change_path), _hasFifo(false), _hasSocket(false) {
    generateFile("regular");
    fs::create_directory("directory");
    if (is_symlink_creation_supported()) {
      fs::create_symlink("regular", "file_symlink");
      fs::create_directory_symlink("directory", "dir_symlink");
    }
#if !defined(TURBO_PLATFORM_WINDOWS) && !defined(TURBO_PLATFORM_WEB)
    EXPECT_TRUE(::mkfifo("fifo", 0644) == 0);
    _hasFifo = true;
    struct ::sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, "socket", sizeof(addr.sun_path));
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    bind(fd, (struct sockaddr *)&addr, sizeof addr);
    _hasSocket = true;
#endif
  }

  ~FileTypeMixFixture() {}

  bool has_fifo() const { return _hasFifo; }

  bool has_socket() const { return _hasSocket; }

  fs::path block_path() const {
    std::error_code ec;
    if (fs::exists("/dev/sda", ec)) {
      return "/dev/sda";
    } else if (fs::exists("/dev/disk0", ec)) {
      return "/dev/disk0";
    }
    return fs::path();
  }

  fs::path character_path() const {
    std::error_code ec;
    if (fs::exists("/dev/null", ec)) {
      return "/dev/null";
    } else if (fs::exists("NUL", ec)) {
      return "NUL";
    }
    return fs::path();
  }
  fs::path temp_path() const { return _t.path(); }

private:
  TemporaryDirectory _t;
  bool _hasFifo;
  bool _hasSocket;
};

/*
TEST_F(FileTypeMixFixture, is_block_file) {
    std::error_code ec;
    ASSERT_TRUE(!fs::is_block_file("directory"));
    ASSERT_TRUE(!fs::is_block_file("regular"));
    if (is_symlink_creation_supported()) {
        ASSERT_TRUE(!fs::is_block_file("dir_symlink"));
        ASSERT_TRUE(!fs::is_block_file("file_symlink"));
    }
    ASSERT_TRUE((has_fifo() ? !fs::is_block_file("fifo") : true));
    ASSERT_TRUE((has_socket() ? !fs::is_block_file("socket") : true));
    ASSERT_TRUE((block_path().empty() ? true :
fs::is_block_file(block_path()))); ASSERT_TRUE((character_path().empty() ? true
: !fs::is_block_file(character_path())));
    ASSERT_NO_THROW(fs::is_block_file("notfound"));
    ASSERT_NO_THROW(fs::is_block_file("notfound", ec));
    ASSERT_TRUE(ec);
    ec.clear();
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::none)));
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::not_found)));
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::regular)));
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::directory)));
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::symlink)));
    ASSERT_TRUE(fs::is_block_file(fs::file_status(fs::file_type::block)));
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::character)));
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::fifo)));
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::socket)));
    ASSERT_TRUE(!fs::is_block_file(fs::file_status(fs::file_type::unknown)));
}
*/
/*
TEST_CASE_METHOD(FileTypeMixFixture, "fs.op.is_character_file -
is_character_file", "[filesystem][operations][fs.op.is_character_file]")
{
    std::error_code ec;
    ASSERT_TRUE(!fs::is_character_file("directory"));
    ASSERT_TRUE(!fs::is_character_file("regular"));
    if (is_symlink_creation_supported()) {
        ASSERT_TRUE(!fs::is_character_file("dir_symlink"));
        ASSERT_TRUE(!fs::is_character_file("file_symlink"));
    }
    ASSERT_TRUE((has_fifo() ? !fs::is_character_file("fifo") : true));
    ASSERT_TRUE((has_socket() ? !fs::is_character_file("socket") : true));
    ASSERT_TRUE((block_path().empty() ? true :
!fs::is_character_file(block_path()))); ASSERT_TRUE((character_path().empty() ?
true : fs::is_character_file(character_path())));
    ASSERT_NO_THROW(fs::is_character_file("notfound"));
    ASSERT_NO_THROW(fs::is_character_file("notfound", ec));
    ASSERT_TRUE(ec);
    ec.clear();
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::none)));
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::not_found)));
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::regular)));
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::directory)));
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::symlink)));
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::block)));
    ASSERT_TRUE(fs::is_character_file(fs::file_status(fs::file_type::character)));
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::fifo)));
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::socket)));
    ASSERT_TRUE(!fs::is_character_file(fs::file_status(fs::file_type::unknown)));
}

TEST_CASE_METHOD(FileTypeMixFixture, "fs.op.is_directory - is_directory",
"[filesystem][operations][fs.op.is_directory]")
{
    std::error_code ec;
    ASSERT_TRUE(fs::is_directory("directory"));
    ASSERT_TRUE(!fs::is_directory("regular"));
    if (is_symlink_creation_supported()) {
        ASSERT_TRUE(fs::is_directory("dir_symlink"));
        ASSERT_TRUE(!fs::is_directory("file_symlink"));
    }
    ASSERT_TRUE((has_fifo() ? !fs::is_directory("fifo") : true));
    ASSERT_TRUE((has_socket() ? !fs::is_directory("socket") : true));
    ASSERT_TRUE((block_path().empty() ? true :
!fs::is_directory(block_path()))); ASSERT_TRUE((character_path().empty() ? true
: !fs::is_directory(character_path())));
    ASSERT_NO_THROW(fs::is_directory("notfound"));
    ASSERT_NO_THROW(fs::is_directory("notfound", ec));
    ASSERT_TRUE(ec);
    ec.clear();
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::none)));
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::not_found)));
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::regular)));
    ASSERT_TRUE(fs::is_directory(fs::file_status(fs::file_type::directory)));
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::symlink)));
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::block)));
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::character)));
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::fifo)));
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::socket)));
    ASSERT_TRUE(!fs::is_directory(fs::file_status(fs::file_type::unknown)));
}
*/
TEST(Filesystem, is_empty) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  ASSERT_TRUE(fs::is_empty(t.path()));
  ASSERT_TRUE(fs::is_empty(t.path(), ec));
  ASSERT_TRUE(!ec);
  generateFile("foo", 0);
  generateFile("bar", 1234);
  ASSERT_TRUE(fs::is_empty("foo"));
  ASSERT_TRUE(fs::is_empty("foo", ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(!fs::is_empty("bar"));
  ASSERT_TRUE(!fs::is_empty("bar", ec));
  ASSERT_TRUE(!ec);
  ASSERT_THROW(fs::is_empty("foobar"), fs::filesystem_error);
  bool result = false;
  ASSERT_NO_THROW(result = fs::is_empty("foobar", ec));
  ASSERT_TRUE(!result);
  ASSERT_TRUE(ec);
}

/*
TEST_CASE_METHOD(FileTypeMixFixture, "fs.op.is_fifo - is_fifo",
"[filesystem][operations][fs.op.is_fifo]")
{
    std::error_code ec;
    ASSERT_TRUE(!fs::is_fifo("directory"));
    ASSERT_TRUE(!fs::is_fifo("regular"));
    if (is_symlink_creation_supported()) {
        ASSERT_TRUE(!fs::is_fifo("dir_symlink"));
        ASSERT_TRUE(!fs::is_fifo("file_symlink"));
    }
    ASSERT_TRUE((has_fifo() ? fs::is_fifo("fifo") : true));
    ASSERT_TRUE((has_socket() ? !fs::is_fifo("socket") : true));
    ASSERT_TRUE((block_path().empty() ? true : !fs::is_fifo(block_path())));
    ASSERT_TRUE((character_path().empty() ? true :
!fs::is_fifo(character_path()))); ASSERT_NO_THROW(fs::is_fifo("notfound"));
    ASSERT_NO_THROW(fs::is_fifo("notfound", ec));
    ASSERT_TRUE(ec);
    ec.clear();
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::none)));
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::not_found)));
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::regular)));
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::directory)));
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::symlink)));
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::block)));
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::character)));
    ASSERT_TRUE(fs::is_fifo(fs::file_status(fs::file_type::fifo)));
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::socket)));
    ASSERT_TRUE(!fs::is_fifo(fs::file_status(fs::file_type::unknown)));
}
*/
/*
TEST_CASE_METHOD(FileTypeMixFixture, "fs.op.is_other - is_other",
"[filesystem][operations][fs.op.is_other]")
{
    std::error_code ec;
    ASSERT_TRUE(!fs::is_other("directory"));
    ASSERT_TRUE(!fs::is_other("regular"));
    if (is_symlink_creation_supported()) {
        ASSERT_TRUE(!fs::is_other("dir_symlink"));
        ASSERT_TRUE(!fs::is_other("file_symlink"));
    }
    ASSERT_TRUE((has_fifo() ? fs::is_other("fifo") : true));
    ASSERT_TRUE((has_socket() ? fs::is_other("socket") : true));
    ASSERT_TRUE((block_path().empty() ? true : fs::is_other(block_path())));
    ASSERT_TRUE((character_path().empty() ? true :
fs::is_other(character_path()))); ASSERT_NO_THROW(fs::is_other("notfound"));
    ASSERT_NO_THROW(fs::is_other("notfound", ec));
    ASSERT_TRUE(ec);
    ec.clear();
    ASSERT_TRUE(!fs::is_other(fs::file_status(fs::file_type::none)));
    ASSERT_TRUE(!fs::is_other(fs::file_status(fs::file_type::not_found)));
    ASSERT_TRUE(!fs::is_other(fs::file_status(fs::file_type::regular)));
    ASSERT_TRUE(!fs::is_other(fs::file_status(fs::file_type::directory)));
    ASSERT_TRUE(!fs::is_other(fs::file_status(fs::file_type::symlink)));
    ASSERT_TRUE(fs::is_other(fs::file_status(fs::file_type::block)));
    ASSERT_TRUE(fs::is_other(fs::file_status(fs::file_type::character)));
    ASSERT_TRUE(fs::is_other(fs::file_status(fs::file_type::fifo)));
    ASSERT_TRUE(fs::is_other(fs::file_status(fs::file_type::socket)));
    ASSERT_TRUE(fs::is_other(fs::file_status(fs::file_type::unknown)));
}
*/
/*
TEST_CASE_METHOD(FileTypeMixFixture, "fs.op.is_regular_file - is_regular_file",
"[filesystem][operations][fs.op.is_regular_file]")
{
    std::error_code ec;
    ASSERT_TRUE(!fs::is_regular_file("directory"));
    ASSERT_TRUE(fs::is_regular_file("regular"));
    if (is_symlink_creation_supported()) {
        ASSERT_TRUE(!fs::is_regular_file("dir_symlink"));
        ASSERT_TRUE(fs::is_regular_file("file_symlink"));
    }
    ASSERT_TRUE((has_fifo() ? !fs::is_regular_file("fifo") : true));
    ASSERT_TRUE((has_socket() ? !fs::is_regular_file("socket") : true));
    ASSERT_TRUE((block_path().empty() ? true :
!fs::is_regular_file(block_path()))); ASSERT_TRUE((character_path().empty() ?
true : !fs::is_regular_file(character_path())));
    ASSERT_NO_THROW(fs::is_regular_file("notfound"));
    ASSERT_NO_THROW(fs::is_regular_file("notfound", ec));
    ASSERT_TRUE(ec);
    ec.clear();
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::none)));
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::not_found)));
    ASSERT_TRUE(fs::is_regular_file(fs::file_status(fs::file_type::regular)));
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::directory)));
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::symlink)));
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::block)));
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::character)));
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::fifo)));
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::socket)));
    ASSERT_TRUE(!fs::is_regular_file(fs::file_status(fs::file_type::unknown)));
}
 */
/*
TEST_CASE_METHOD(FileTypeMixFixture, "fs.op.is_socket - is_socket",
"[filesystem][operations][fs.op.is_socket]")
{
    std::error_code ec;
    ASSERT_TRUE(!fs::is_socket("directory"));
    ASSERT_TRUE(!fs::is_socket("regular"));
    if (is_symlink_creation_supported()) {
        ASSERT_TRUE(!fs::is_socket("dir_symlink"));
        ASSERT_TRUE(!fs::is_socket("file_symlink"));
    }
    ASSERT_TRUE((has_fifo() ? !fs::is_socket("fifo") : true));
    ASSERT_TRUE((has_socket() ? fs::is_socket("socket") : true));
    ASSERT_TRUE((block_path().empty() ? true : !fs::is_socket(block_path())));
    ASSERT_TRUE((character_path().empty() ? true :
!fs::is_socket(character_path()))); ASSERT_NO_THROW(fs::is_socket("notfound"));
    ASSERT_NO_THROW(fs::is_socket("notfound", ec));
    ASSERT_TRUE(ec);
    ec.clear();
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::none)));
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::not_found)));
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::regular)));
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::directory)));
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::symlink)));
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::block)));
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::character)));
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::fifo)));
    ASSERT_TRUE(fs::is_socket(fs::file_status(fs::file_type::socket)));
    ASSERT_TRUE(!fs::is_socket(fs::file_status(fs::file_type::unknown)));
}
*/
/*
TEST_CASE_METHOD(FileTypeMixFixture, "fs.op.is_symlink - is_symlink",
"[filesystem][operations][fs.op.is_symlink]")
{
    std::error_code ec;
    ASSERT_TRUE(!fs::is_symlink("directory"));
    ASSERT_TRUE(!fs::is_symlink("regular"));
    if (is_symlink_creation_supported()) {
        ASSERT_TRUE(fs::is_symlink("dir_symlink"));
        ASSERT_TRUE(fs::is_symlink("file_symlink"));
    }
    ASSERT_TRUE((has_fifo() ? !fs::is_symlink("fifo") : true));
    ASSERT_TRUE((has_socket() ? !fs::is_symlink("socket") : true));
    ASSERT_TRUE((block_path().empty() ? true : !fs::is_symlink(block_path())));
    ASSERT_TRUE((character_path().empty() ? true :
!fs::is_symlink(character_path())));
    ASSERT_NO_THROW(fs::is_symlink("notfound"));
    ASSERT_NO_THROW(fs::is_symlink("notfound", ec));
    ASSERT_TRUE(ec);
    ec.clear();
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::none)));
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::not_found)));
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::regular)));
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::directory)));
    ASSERT_TRUE(fs::is_symlink(fs::file_status(fs::file_type::symlink)));
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::block)));
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::character)));
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::fifo)));
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::socket)));
    ASSERT_TRUE(!fs::is_symlink(fs::file_status(fs::file_type::unknown)));
}
*/
#ifndef TURBO_PLATFORM_WEB
static fs::file_time_type timeFromString(const std::string &str) {
  struct ::tm tm;
  ::memset(&tm, 0, sizeof(::tm));
  std::istringstream is(str);
  is >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (is.fail()) {
    throw std::exception();
  }
  return from_time_t<fs::file_time_type>(std::mktime(&tm));
}
#endif

TEST(Filesystem, last_write_time) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  fs::file_time_type ft;
  generateFile("foo");
  auto now = fs::file_time_type::clock::now();
  ASSERT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(
                           fs::last_write_time(t.path()) - now)
                           .count()) < 3);
  ASSERT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(
                           fs::last_write_time("foo") - now)
                           .count()) < 3);
  ASSERT_THROW(fs::last_write_time("bar"), fs::filesystem_error);
  ASSERT_NO_THROW(ft = fs::last_write_time("bar", ec));
  ASSERT_TRUE(ft == fs::file_time_type::min());
  ASSERT_TRUE(ec);
  ec.clear();
  if (is_symlink_creation_supported()) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    fs::create_symlink("foo", "foo2");
    ft = fs::last_write_time("foo");
    // checks that the time of the symlink is fetched
    ASSERT_TRUE(ft == fs::last_write_time("foo2"));
  }
#ifndef TURBO_PLATFORM_WEB
  auto nt = timeFromString("2015-10-21T04:30:00");
  ASSERT_NO_THROW(fs::last_write_time(t.path() / "foo", nt));
  ASSERT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(
                           fs::last_write_time("foo") - nt)
                           .count()) < 1);
  nt = timeFromString("2015-10-21T04:29:00");
  ASSERT_NO_THROW(fs::last_write_time("foo", nt, ec));
  ASSERT_TRUE(std::abs(std::chrono::duration_cast<std::chrono::seconds>(
                           fs::last_write_time("foo") - nt)
                           .count()) < 1);
  ASSERT_TRUE(!ec);
  ASSERT_THROW(fs::last_write_time("bar", nt), fs::filesystem_error);
  ASSERT_NO_THROW(fs::last_write_time("bar", nt, ec));
  ASSERT_TRUE(ec);
#endif
}

TEST(Filesystem, permissions) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo", 512);
  auto allWrite =
      fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write;
  ASSERT_NO_THROW(fs::permissions("foo", allWrite, fs::perm_options::remove));
  ASSERT_TRUE((fs::status("foo").permissions() & fs::perms::owner_write) !=
              fs::perms::owner_write);
#if !defined(TURBO_PLATFORM_WINDOWS)
  if (geteuid() != 0)
#endif
  {
    ASSERT_THROW(fs::resize_file("foo", 1024), fs::filesystem_error);
    ASSERT_TRUE(fs::file_size("foo") == 512);
  }
  ASSERT_NO_THROW(
      fs::permissions("foo", fs::perms::owner_write, fs::perm_options::add));
  ASSERT_TRUE((fs::status("foo").permissions() & fs::perms::owner_write) ==
              fs::perms::owner_write);
  ASSERT_NO_THROW(fs::resize_file("foo", 2048));
  ASSERT_TRUE(fs::file_size("foo") == 2048);
  ASSERT_THROW(
      fs::permissions("bar", fs::perms::owner_write, fs::perm_options::add),
      fs::filesystem_error);
  ASSERT_NO_THROW(fs::permissions("bar", fs::perms::owner_write,
                                  fs::perm_options::add, ec));
  ASSERT_TRUE(ec);
  ASSERT_THROW(fs::permissions("bar", fs::perms::owner_write,
                               static_cast<fs::perm_options>(0)),
               fs::filesystem_error);
}

TEST(Filesystem, proximate) {
  std::error_code ec;
  ASSERT_TRUE(fs::proximate("/a/d", "/a/b/c") == "../../d");
  ASSERT_TRUE(fs::proximate("/a/d", "/a/b/c", ec) == "../../d");
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::proximate("/a/b/c", "/a/d") == "../b/c");
  ASSERT_TRUE(fs::proximate("/a/b/c", "/a/d", ec) == "../b/c");
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::proximate("a/b/c", "a") == "b/c");
  ASSERT_TRUE(fs::proximate("a/b/c", "a", ec) == "b/c");
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::proximate("a/b/c", "a/b/c/x/y") == "../..");
  ASSERT_TRUE(fs::proximate("a/b/c", "a/b/c/x/y", ec) == "../..");
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::proximate("a/b/c", "a/b/c") == ".");
  ASSERT_TRUE(fs::proximate("a/b/c", "a/b/c", ec) == ".");
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::proximate("a/b", "c/d") == "../../a/b");
  ASSERT_TRUE(fs::proximate("a/b", "c/d", ec) == "../../a/b");
  ASSERT_TRUE(!ec);
#ifndef TURBO_PLATFORM_WINDOWS
  if (has_host_root_name_support()) {
    ASSERT_TRUE(fs::proximate("//host1/a/d", "//host2/a/b/c") == "//host1/a/d");
    ASSERT_TRUE(fs::proximate("//host1/a/d", "//host2/a/b/c", ec) ==
                "//host1/a/d");
    ASSERT_TRUE(!ec);
  }
#endif
}

TEST(Filesystem, read_symlink) {
  if (is_symlink_creation_supported()) {
    TemporaryDirectory t(TempOpt::change_path);
    std::error_code ec;
    generateFile("foo");
    fs::create_symlink(t.path() / "foo", "bar");
    ASSERT_TRUE(fs::read_symlink("bar") == t.path() / "foo");
    ASSERT_TRUE(fs::read_symlink("bar", ec) == t.path() / "foo");
    ASSERT_TRUE(!ec);
    ASSERT_THROW(fs::read_symlink("foobar"), fs::filesystem_error);
    ASSERT_TRUE(fs::read_symlink("foobar", ec) == fs::path());
    ASSERT_TRUE(ec);
  }
}

TEST(Filesystem, fs_op_relative) {
  ASSERT_TRUE(fs::relative("/a/d", "/a/b/c") == "../../d");
  ASSERT_TRUE(fs::relative("/a/b/c", "/a/d") == "../b/c");
  ASSERT_TRUE(fs::relative("a/b/c", "a") == "b/c");
  ASSERT_TRUE(fs::relative("a/b/c", "a/b/c/x/y") == "../..");
  ASSERT_TRUE(fs::relative("a/b/c", "a/b/c") == ".");
  ASSERT_TRUE(fs::relative("a/b", "c/d") == "../../a/b");
  std::error_code ec;
  ASSERT_TRUE(fs::relative(fs::current_path() / "foo", ec) == "foo");
  ASSERT_TRUE(!ec);
}

TEST(filesystem, remove) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo");
  ASSERT_TRUE(fs::remove("foo"));
  ASSERT_TRUE(!fs::exists("foo"));
  ASSERT_TRUE(!fs::remove("foo"));
  generateFile("foo");
  ASSERT_TRUE(fs::remove("foo", ec));
  ASSERT_TRUE(!fs::exists("foo"));
  if (is_symlink_creation_supported()) {
    generateFile("foo");
    fs::create_symlink("foo", "bar");
    ASSERT_TRUE(fs::exists(fs::symlink_status("bar")));
    ASSERT_TRUE(fs::remove("bar", ec));
    ASSERT_TRUE(fs::exists("foo"));
    ASSERT_TRUE(!fs::exists(fs::symlink_status("bar")));
  }
  ASSERT_TRUE(!fs::remove("bar"));
  ASSERT_TRUE(!fs::remove("bar", ec));
  ASSERT_TRUE(!ec);
}

TEST(Filesystem, remove_all) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo");
  ASSERT_TRUE(fs::remove_all("foo", ec) == 1);
  ASSERT_TRUE(!ec);
  ec.clear();
  ASSERT_TRUE(fs::directory_iterator(t.path()) == fs::directory_iterator());
  fs::create_directories("dir1/dir1a");
  fs::create_directories("dir1/dir1b");
  generateFile("dir1/dir1a/f1");
  generateFile("dir1/dir1b/f2");
  ASSERT_NO_THROW(fs::remove_all("dir1/non-existing", ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::remove_all("dir1/non-existing", ec) == 0);
  if (is_symlink_creation_supported()) {
    fs::create_directory_symlink("dir1", "dir1link");
    ASSERT_TRUE(fs::remove_all("dir1link") == 1);
  }
  ASSERT_TRUE(fs::remove_all("dir1") == 5);
  ASSERT_TRUE(fs::directory_iterator(t.path()) == fs::directory_iterator());
}

TEST(Filesystem, rename) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo", 123);
  fs::create_directory("dir1");
  ASSERT_NO_THROW(fs::rename("foo", "bar"));
  ASSERT_TRUE(!fs::exists("foo"));
  ASSERT_TRUE(fs::exists("bar"));
  ASSERT_NO_THROW(fs::rename("dir1", "dir2"));
  ASSERT_TRUE(fs::exists("dir2"));
  generateFile("foo2", 42);
  ASSERT_NO_THROW(fs::rename("bar", "foo2"));
  ASSERT_TRUE(fs::exists("foo2"));
  ASSERT_TRUE(fs::file_size("foo2") == 123u);
  ASSERT_TRUE(!fs::exists("bar"));
  ASSERT_NO_THROW(fs::rename("foo2", "foo", ec));
  ASSERT_TRUE(!ec);
  ASSERT_THROW(fs::rename("foobar", "barfoo"), fs::filesystem_error);
  ASSERT_NO_THROW(fs::rename("foobar", "barfoo", ec));
  ASSERT_TRUE(ec);
  ASSERT_TRUE(!fs::exists("barfoo"));
}

TEST(Filesystem, resize_file) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo", 1024);
  ASSERT_TRUE(fs::file_size("foo") == 1024);
  ASSERT_NO_THROW(fs::resize_file("foo", 2048));
  ASSERT_TRUE(fs::file_size("foo") == 2048);
  ASSERT_NO_THROW(fs::resize_file("foo", 1000, ec));
  ASSERT_TRUE(!ec);
  ASSERT_TRUE(fs::file_size("foo") == 1000);
  ASSERT_THROW(fs::resize_file("bar", 2048), fs::filesystem_error);
  ASSERT_TRUE(!fs::exists("bar"));
  ASSERT_NO_THROW(fs::resize_file("bar", 4096, ec));
  ASSERT_TRUE(ec);
  ASSERT_TRUE(!fs::exists("bar"));
}

TEST(Filesystem, fs_op_space) {
  {
    fs::space_info si;
    ASSERT_NO_THROW(si = fs::space(fs::current_path()));
    ASSERT_TRUE(si.capacity > 1024 * 1024);
    ASSERT_TRUE(si.capacity > si.free);
    ASSERT_TRUE(si.free >= si.available);
  }
  {
    std::error_code ec;
    fs::space_info si;
    ASSERT_NO_THROW(si = fs::space(fs::current_path(), ec));
    ASSERT_TRUE(si.capacity > 1024 * 1024);
    ASSERT_TRUE(si.capacity > si.free);
    ASSERT_TRUE(si.free >= si.available);
    ASSERT_TRUE(!ec);
  }
#ifndef TURBO_PLATFORM_WEB // statvfs under emscripten always returns a result,
                           // so this tests would fail
  {
    std::error_code ec;
    fs::space_info si;
    ASSERT_NO_THROW(si = fs::space("foobar42", ec));
    ASSERT_TRUE(si.capacity == static_cast<uintmax_t>(-1));
    ASSERT_TRUE(si.free == static_cast<uintmax_t>(-1));
    ASSERT_TRUE(si.available == static_cast<uintmax_t>(-1));
    ASSERT_TRUE(ec);
  }
  ASSERT_THROW(fs::space("foobar42"), fs::filesystem_error);
#endif
}

TEST(Filesystem, op_and_status) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  fs::file_status fs;
  ASSERT_NO_THROW(fs = fs::status("foo"));
  ASSERT_TRUE(fs.type() == fs::file_type::not_found);
  ASSERT_TRUE(fs.permissions() == fs::perms::unknown);
  ASSERT_NO_THROW(fs = fs::status("bar", ec));
  ASSERT_TRUE(fs.type() == fs::file_type::not_found);
  ASSERT_TRUE(fs.permissions() == fs::perms::unknown);
  ASSERT_TRUE(ec);
  ec.clear();
  fs = fs::status(t.path());
  ASSERT_TRUE(fs.type() == fs::file_type::directory);
  ASSERT_TRUE(
      (fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write)) ==
      (fs::perms::owner_read | fs::perms::owner_write));
  generateFile("foobar");
  fs = fs::status(t.path() / "foobar");
  ASSERT_TRUE(fs.type() == fs::file_type::regular);
  ASSERT_TRUE(
      (fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write)) ==
      (fs::perms::owner_read | fs::perms::owner_write));
  if (is_symlink_creation_supported()) {
    fs::create_symlink(t.path() / "foobar", t.path() / "barfoo");
    fs = fs::status(t.path() / "barfoo");
    ASSERT_TRUE(fs.type() == fs::file_type::regular);
    ASSERT_TRUE(
        (fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write)) ==
        (fs::perms::owner_read | fs::perms::owner_write));
  }
}

TEST(FilesystemStatus, status_known) {
  ASSERT_TRUE(!fs::status_known(fs::file_status()));
  ASSERT_TRUE(fs::status_known(fs::file_status(fs::file_type::not_found)));
  ASSERT_TRUE(fs::status_known(fs::file_status(fs::file_type::regular)));
  ASSERT_TRUE(fs::status_known(fs::file_status(fs::file_type::directory)));
  ASSERT_TRUE(fs::status_known(fs::file_status(fs::file_type::symlink)));
  ASSERT_TRUE(fs::status_known(fs::file_status(fs::file_type::character)));
  ASSERT_TRUE(fs::status_known(fs::file_status(fs::file_type::fifo)));
  ASSERT_TRUE(fs::status_known(fs::file_status(fs::file_type::socket)));
  ASSERT_TRUE(fs::status_known(fs::file_status(fs::file_type::unknown)));
}

TEST(FilesystemStatus, symlink_status) {
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  fs::file_status fs;
  ASSERT_NO_THROW(fs = fs::symlink_status("foo"));
  ASSERT_TRUE(fs.type() == fs::file_type::not_found);
  ASSERT_TRUE(fs.permissions() == fs::perms::unknown);
  ASSERT_NO_THROW(fs = fs::symlink_status("bar", ec));
  ASSERT_TRUE(fs.type() == fs::file_type::not_found);
  ASSERT_TRUE(fs.permissions() == fs::perms::unknown);
  ASSERT_TRUE(ec);
  ec.clear();
  fs = fs::symlink_status(t.path());
  ASSERT_TRUE(fs.type() == fs::file_type::directory);
  ASSERT_TRUE(
      (fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write)) ==
      (fs::perms::owner_read | fs::perms::owner_write));
  generateFile("foobar");
  fs = fs::symlink_status(t.path() / "foobar");
  ASSERT_TRUE(fs.type() == fs::file_type::regular);
  ASSERT_TRUE(
      (fs.permissions() & (fs::perms::owner_read | fs::perms::owner_write)) ==
      (fs::perms::owner_read | fs::perms::owner_write));
  if (is_symlink_creation_supported()) {
    fs::create_symlink(t.path() / "foobar", t.path() / "barfoo");
    fs = fs::symlink_status(t.path() / "barfoo");
    ASSERT_TRUE(fs.type() == fs::file_type::symlink);
  }
}

TEST(FilesystemStatus, temp_dir_path) {
  std::error_code ec;
  ASSERT_NO_THROW(fs::exists(fs::temp_directory_path()));
  ASSERT_NO_THROW(fs::exists(fs::temp_directory_path(ec)));
  ASSERT_TRUE(!fs::temp_directory_path().empty());
  ASSERT_TRUE(!ec);
}

TEST(FilesystemStatus, weakly_canonical) {
  TURBO_LOG(INFO) << "This might fail on std::implementations that return "
                     "fs::current_path() for fs::canonical(\"\")";
  ASSERT_TRUE(fs::weakly_canonical("") == ".");
  if (fs::weakly_canonical("") == ".") {
    ASSERT_TRUE(fs::weakly_canonical("foo/bar") == "foo/bar");
    ASSERT_TRUE(fs::weakly_canonical("foo/./bar") == "foo/bar");
    ASSERT_TRUE(fs::weakly_canonical("foo/../bar") == "bar");
  } else {
    ASSERT_TRUE(fs::weakly_canonical("foo/bar") ==
                fs::current_path() / "foo/bar");
    ASSERT_TRUE(fs::weakly_canonical("foo/./bar") ==
                fs::current_path() / "foo/bar");
    ASSERT_TRUE(fs::weakly_canonical("foo/../bar") ==
                fs::current_path() / "bar");
  }

  {
    TemporaryDirectory t(TempOpt::change_path);
    auto dir = t.path() / "d0";
    fs::create_directories(dir / "d1");
    generateFile(dir / "f0");
    fs::path rel(dir.filename());
    ASSERT_TRUE(fs::weakly_canonical(dir) == dir);
    ASSERT_TRUE(fs::weakly_canonical(rel) == dir);
    ASSERT_TRUE(fs::weakly_canonical(dir / "f0") == dir / "f0");
    ASSERT_TRUE(fs::weakly_canonical(dir / "f0/") == dir / "f0/");
    ASSERT_TRUE(fs::weakly_canonical(dir / "f1") == dir / "f1");
    ASSERT_TRUE(fs::weakly_canonical(rel / "f0") == dir / "f0");
    ASSERT_TRUE(fs::weakly_canonical(rel / "f0/") == dir / "f0/");
    ASSERT_TRUE(fs::weakly_canonical(rel / "f1") == dir / "f1");
    ASSERT_TRUE(fs::weakly_canonical(rel / "./f0") == dir / "f0");
    ASSERT_TRUE(fs::weakly_canonical(rel / "./f1") == dir / "f1");
    ASSERT_TRUE(fs::weakly_canonical(rel / "d1/../f0") == dir / "f0");
    ASSERT_TRUE(fs::weakly_canonical(rel / "d1/../f1") == dir / "f1");
    ASSERT_TRUE(fs::weakly_canonical(rel / "d1/../f1/../f2") == dir / "f2");
  }
}

TEST(FilesystemStatus, string_view) {

  using turbo::string_view;
  using turbo::wstring_view;
  {
    std::string p("foo/bar");
    string_view sv(p);
    ASSERT_TRUE(
        fs::path(sv, fs::path::format::generic_format).generic_string() ==
        "foo/bar");
    fs::path p2("fo");
    p2 += string_view("o");
    ASSERT_TRUE(p2 == "foo");
    ASSERT_TRUE(p2.compare(string_view("foo")) == 0);
  }
  {
    auto p = fs::path{"XYZ"};
    p /= string_view("Appendix");
    ASSERT_TRUE(p == "XYZ/Appendix");
  }
  {
    std::wstring p(L"foo/bar");
    wstring_view sv(p);
    ASSERT_TRUE(
        fs::path(sv, fs::path::format::generic_format).generic_string() ==
        "foo/bar");
    fs::path p2(L"fo");
    p2 += wstring_view(L"o");
    ASSERT_TRUE(p2 == "foo");
    ASSERT_TRUE(p2.compare(wstring_view(L"foo")) == 0);
  }
}

TEST(FilesystemStatus, win_long) {
#ifdef TURBO_PLATFORM_WINDOWS
  TemporaryDirectory t(TempOpt::change_path);
  char c = 'A';
  fs::path dir{"\\\\?\\"};
  dir += fs::current_path().u8string();
  for (; c <= 'Z'; ++c) {
    std::string part = std::string(16, c);
    dir /= part;
    ASSERT_NO_THROW(fs::create_directory(dir));
    ASSERT_TRUE(fs::exists(dir));
    generateFile(dir / "f0");
    ASSERT_TRUE(fs::exists(dir / "f0"));
  }
  ASSERT_TRUE(c > 'Z');
  fs::remove_all(fs::current_path() / std::string(16, 'A'));
  ASSERT_TRUE(!fs::exists(fs::current_path() / std::string(16, 'A')));
  ASSERT_NO_THROW(fs::create_directories(dir));
  ASSERT_TRUE(fs::exists(dir));
  generateFile(dir / "f0");
  ASSERT_TRUE(fs::exists(dir / "f0"));
#else
  TURBO_LOG(WARNING)
      << "Windows specific tests are empty on non-Windows systems.";
#endif
}

TEST(Filesystem, win_namespaces) {
#ifdef TURBO_PLATFORM_WINDOWS
  {
    std::error_code ec;
    fs::path p(R"(\\localhost\c$\Windows)");
    auto symstat = fs::symlink_status(p, ec);
    ASSERT_TRUE(!ec);
    auto p2 = fs::canonical(p, ec);
    ASSERT_TRUE(!ec);
    ASSERT_TRUE(p2 == p);
  }

  struct TestInfo {
    std::string _path;
    std::string _string;
    std::string _rootName;
    std::string _rootPath;
    std::string _iterateResult;
  };
  std::vector<TestInfo> variants = {
      {R"(C:\Windows\notepad.exe)", R"(C:\Windows\notepad.exe)", "C:", "C:\\",
       "C:,/,Windows,notepad.exe"},
#ifdef USE_STD_FS
      {R"(\\?\C:\Windows\notepad.exe)", R"(\\?\C:\Windows\notepad.exe)",
       "\\\\?", "\\\\?\\", "//?,/,C:,Windows,notepad.exe"},
      {R"(\??\C:\Windows\notepad.exe)", R"(\??\C:\Windows\notepad.exe)", "\\??",
       "\\??\\", "/??,/,C:,Windows,notepad.exe"},
#else
      {R"(\\?\C:\Windows\notepad.exe)", R"(\\?\C:\Windows\notepad.exe)",
       "C:", "C:\\", "//?/,C:,/,Windows,notepad.exe"},
      {R"(\??\C:\Windows\notepad.exe)", R"(\??\C:\Windows\notepad.exe)",
       "C:", "C:\\", "/?\?/,C:,/,Windows,notepad.exe"},
#endif
      {R"(\\.\C:\Windows\notepad.exe)", R"(\\.\C:\Windows\notepad.exe)",
       "\\\\.", "\\\\.\\", "//.,/,C:,Windows,notepad.exe"},
      {R"(\\?\HarddiskVolume1\Windows\notepad.exe)",
       R"(\\?\HarddiskVolume1\Windows\notepad.exe)", "\\\\?", "\\\\?\\",
       "//?,/,HarddiskVolume1,Windows,notepad.exe"},
      {R"(\\?\Harddisk0Partition1\Windows\notepad.exe)",
       R"(\\?\Harddisk0Partition1\Windows\notepad.exe)", "\\\\?", "\\\\?\\",
       "//?,/,Harddisk0Partition1,Windows,notepad.exe"},
      {R"(\\.\GLOBALROOT\Device\HarddiskVolume1\Windows\notepad.exe)",
       R"(\\.\GLOBALROOT\Device\HarddiskVolume1\Windows\notepad.exe)", "\\\\.",
       "\\\\.\\",
       "//.,/,GLOBALROOT,Device,HarddiskVolume1,Windows,notepad.exe"},
      {R"(\\?\GLOBALROOT\Device\Harddisk0\Partition1\Windows\notepad.exe)",
       R"(\\?\GLOBALROOT\Device\Harddisk0\Partition1\Windows\notepad.exe)",
       "\\\\?", "\\\\?\\",
       "//?,/,GLOBALROOT,Device,Harddisk0,Partition1,Windows,notepad.exe"},
      {R"(\\?\Volume{e8a4a89d-0000-0000-0000-100000000000}\Windows\notepad.exe)",
       R"(\\?\Volume{e8a4a89d-0000-0000-0000-100000000000}\Windows\notepad.exe)",
       "\\\\?", "\\\\?\\",
       "//?,/"
       ",Volume{e8a4a89d-0000-0000-0000-100000000000},Windows,notepad.exe"},
      {R"(\\LOCALHOST\C$\Windows\notepad.exe)",
       R"(\\LOCALHOST\C$\Windows\notepad.exe)", "\\\\LOCALHOST",
       "\\\\LOCALHOST\\", "//LOCALHOST,/,C$,Windows,notepad.exe"},
      {R"(\\?\UNC\C$\Windows\notepad.exe)", R"(\\?\UNC\C$\Windows\notepad.exe)",
       "\\\\?", "\\\\?\\", "//?,/,UNC,C$,Windows,notepad.exe"},
      {R"(\\?\GLOBALROOT\Device\Mup\C$\Windows\notepad.exe)",
       R"(\\?\GLOBALROOT\Device\Mup\C$\Windows\notepad.exe)", "\\\\?",
       "\\\\?\\", "//?,/,GLOBALROOT,Device,Mup,C$,Windows,notepad.exe"},
  };

  for (auto ti : variants) {
    INFO("Used path: " + ti._path);
    auto p = fs::path(ti._path);
    ASSERT_TRUE(p.string() == ti._string);
    ASSERT_TRUE(p.is_absolute());
    ASSERT_TRUE(p.root_name().string() == ti._rootName);
    ASSERT_TRUE(p.root_path().string() == ti._rootPath);
    ASSERT_TRUE(iterateResult(p) == ti._iterateResult);
  }
#else
  TURBO_LOG(WARNING)
      << "Windows specific tests are empty on non-Windows systems.";
#endif
}

TEST(Filesystem, win_mapped) {
#ifdef TURBO_PLATFORM_WINDOWS
  // this test expects a mapped volume on C:\\fs-test as is the case on the
  // development test system does nothing on other systems
  if (fs::exists("C:\\fs-test")) {
    ASSERT_TRUE(fs::canonical("C:\\fs-test\\Test.txt").string() ==
                "C:\\fs-test\\Test.txt");
  }
#else
  TURBO_LOG(WARNING)
      << "Windows specific tests are empty on non-Windows systems.";
#endif
}

TEST(Filesystem, win_remove) {
#ifdef TURBO_PLATFORM_WINDOWS
  TemporaryDirectory t(TempOpt::change_path);
  std::error_code ec;
  generateFile("foo", 512);
  auto allWrite =
      fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write;
  ASSERT_NO_THROW(fs::permissions("foo", allWrite, fs::perm_options::remove));
  ASSERT_NO_THROW(fs::remove("foo"));
  ASSERT_TRUE(!fs::exists("foo"));
#else
  TURBO_LOG(WARNING)
      << "Windows specific tests are empty on non-Windows systems.";
#endif
}
