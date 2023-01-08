
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#include "flare/strings/internal/ostringstream.h"

namespace flare {

namespace strings_internal {

string_output_stream::Buf::int_type string_output_stream::overflow(int c) {
    assert(s_);
    if (!Buf::traits_type::eq_int_type(c, Buf::traits_type::eof()))
        s_->push_back(static_cast<char>(c));
    return 1;
}

std::streamsize string_output_stream::xsputn(const char *s, std::streamsize n) {
    assert(s_);
    s_->append(s, n);
    return n;
}

}  // namespace strings_internal

}  // namespace flare
