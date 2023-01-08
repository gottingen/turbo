
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TEST_TESTING_INTERNAL_TIME_UTIL_H_
#define TEST_TESTING_INTERNAL_TIME_UTIL_H_

#include <string>
#include <cstdarg>
#include <cstdio>
#include <string>
#include "flare/times/time.h"
#include "flare/strings/fmt/os.h"

namespace flare::times_internal {


    // Loads the named timezone, but dies on any failure.
    flare::time_zone load_time_zone(const std::string &name);


}  // namespace flare::times_internal


enum {
    BUFFER_SIZE = 256
};

#ifdef _MSC_VER
# define FMT_VSNPRINTF vsprintf_s
#else
# define FMT_VSNPRINTF vsnprintf
#endif

template<std::size_t SIZE>
void safe_sprintf(char (&buffer)[SIZE], const char *format, ...) {
    std::va_list args;
    va_start(args, format);
    FMT_VSNPRINTF(buffer, SIZE, format, args);
    va_end(args);
}

// Increment a number in a string.
void increment(char *s);

std::string get_system_error(int error_code);

extern const char *const FILE_CONTENT;

// Opens a buffered file for reading.
fmt::buffered_file open_buffered_file(FILE **fp = 0);

inline FILE *safe_fopen(const char *filename, const char *mode) {
#if defined(_WIN32) && !defined(__MINGW32__)
    // Fix MSVC warning about "unsafe" fopen.
  FILE *f = 0;
  errno = fopen_s(&f, filename, mode);
  return f;
#else
    return std::fopen(filename, mode);
#endif
}

template<typename Char>
class BasicTestString {
private:
    std::basic_string<Char> value_;

    static const Char EMPTY[];

public:
    explicit BasicTestString(const Char *value = EMPTY) : value_(value) {}

    const std::basic_string<Char> &value() const { return value_; }
};

template<typename Char>
const Char BasicTestString<Char>::EMPTY[] = {0};

typedef BasicTestString<char> TestString;
typedef BasicTestString<wchar_t> TestWString;

template<typename Char>
std::basic_ostream<Char> &operator<<(
        std::basic_ostream<Char> &os, const BasicTestString<Char> &s) {
    os << s.value();
    return os;
}

class Date {
    int year_, month_, day_;
public:
    Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

    int year() const { return year_; }

    int month() const { return month_; }

    int day() const { return day_; }
};

#endif  // TEST_TESTING_INTERNAL_TIME_UTIL_H_
