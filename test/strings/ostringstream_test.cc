
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/strings/internal/ostringstream.h"

#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

#include "testing/gtest_wrap.h"

namespace {

    TEST(string_output_stream, IsOStream) {
        static_assert(
                std::is_base_of<std::ostream, flare::strings_internal::string_output_stream>(),
                "");
    }

    TEST(string_output_stream, ConstructDestroy) {
        {
            flare::strings_internal::string_output_stream strm(nullptr);
            EXPECT_EQ(nullptr, strm.str());
        }
        {
            std::string s = "abc";
            {
                flare::strings_internal::string_output_stream strm(&s);
                EXPECT_EQ(&s, strm.str());
            }
            EXPECT_EQ("abc", s);
        }
        {
            std::unique_ptr<std::string> s(new std::string);
            flare::strings_internal::string_output_stream strm(s.get());
            s.reset();
        }
    }

    TEST(string_output_stream, Str) {
        std::string s1;
        flare::strings_internal::string_output_stream strm(&s1);
        const flare::strings_internal::string_output_stream &c_strm(strm);

        static_assert(std::is_same<decltype(strm.str()), std::string *>(), "");
        static_assert(std::is_same<decltype(c_strm.str()), const std::string *>(), "");

        EXPECT_EQ(&s1, strm.str());
        EXPECT_EQ(&s1, c_strm.str());

        strm.str(&s1);
        EXPECT_EQ(&s1, strm.str());
        EXPECT_EQ(&s1, c_strm.str());

        std::string s2;
        strm.str(&s2);
        EXPECT_EQ(&s2, strm.str());
        EXPECT_EQ(&s2, c_strm.str());

        strm.str(nullptr);
        EXPECT_EQ(nullptr, strm.str());
        EXPECT_EQ(nullptr, c_strm.str());
    }

    TEST(OStreamStream, WriteToLValue) {
        std::string s = "abc";
        {
            flare::strings_internal::string_output_stream strm(&s);
            EXPECT_EQ("abc", s);
            strm << "";
            EXPECT_EQ("abc", s);
            strm << 42;
            EXPECT_EQ("abc42", s);
            strm << 'x' << 'y';
            EXPECT_EQ("abc42xy", s);
        }
        EXPECT_EQ("abc42xy", s);
    }

    TEST(OStreamStream, WriteToRValue) {
        std::string s = "abc";
        flare::strings_internal::string_output_stream(&s) << "";
        EXPECT_EQ("abc", s);
        flare::strings_internal::string_output_stream(&s) << 42;
        EXPECT_EQ("abc42", s);
        flare::strings_internal::string_output_stream(&s) << 'x' << 'y';
        EXPECT_EQ("abc42xy", s);
    }

}  // namespace
