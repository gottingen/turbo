
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#ifndef FLARE_STRINGS_INTERNAL_OSTRINGSTREAM_H_
#define FLARE_STRINGS_INTERNAL_OSTRINGSTREAM_H_

#include <cassert>
#include <ostream>
#include <streambuf>
#include <string>

#include "flare/base/profile.h"

namespace flare::strings_internal {

    // The same as std::ostringstream but appends to a user-specified std::string,
    // and is faster. It is ~70% faster to create, ~50% faster to write to, and
    // completely free to extract the result std::string.
    //
    //   std::string s;
    //   string_output_stream strm(&s);
    //   strm << 42 << ' ' << 3.14;  // appends to `s`
    //
    // The stream object doesn't have to be named. Starting from C++11 operator<<
    // works with rvalues of std::ostream.
    //
    //   std::string s;
    //   string_output_stream(&s) << 42 << ' ' << 3.14;  // appends to `s`
    //
    // string_output_stream is faster to create than std::ostringstream but it's still
    // relatively slow. Avoid creating multiple streams where a single stream will
    // do.
    //
    // Creates unnecessary instances of string_output_stream: slow.
    //
    //   std::string s;
    //   string_output_stream(&s) << 42;
    //   string_output_stream(&s) << ' ';
    //   string_output_stream(&s) << 3.14;
    //
    // Creates a single instance of string_output_stream and reuses it: fast.
    //
    //   std::string s;
    //   string_output_stream strm(&s);
    //   strm << 42;
    //   strm << ' ';
    //   strm << 3.14;
    //
    // Note: flush() has no effect. No reason to call it.
    class string_output_stream : private std::basic_streambuf<char>, public std::ostream {
    public:
        // The argument can be null, in which case you'll need to call str(p) with a
        // non-null argument before you can write to the stream.
        //
        // The destructor of string_output_stream doesn't use the std::string. It's OK to
        // destroy the std::string before the stream.
        explicit string_output_stream(std::string *s) : std::ostream(this), s_(s) {}

        std::string *str() { return s_; }

        const std::string *str() const { return s_; }

        void str(std::string *s) { s_ = s; }

    private:
        using Buf = std::basic_streambuf<char>;

        Buf::int_type overflow(int c) override;

        std::streamsize xsputn(const char *s, std::streamsize n) override;

        std::string *s_;
    };

}  // namespace flare::strings_internal

#endif  // FLARE_STRINGS_INTERNAL_OSTRINGSTREAM_H_
