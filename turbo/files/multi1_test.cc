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
#include "gtest/gtest.h"
#include "turbo/files/filesystem.h"

namespace fs = ghc::filesystem;

// This test and the one in multi2.cpp doesn't actually test relevant functionality,
// it is just used to check that it is possible to include filesystem.h in multiple
// source files.
TEST(Multifile, multi_1) {
    ASSERT_TRUE("/usr/local/bin" == fs::path("/usr/local/bin").generic_string());
    std::string str = "/usr/local/bin";
    std::u16string u16str = u"/usr/local/bin";
    std::u32string u32str = U"/usr/local/bin";
    ASSERT_TRUE(str == fs::path(str.begin(), str.end()));
    ASSERT_TRUE(str == fs::path(u16str.begin(), u16str.end()));
    ASSERT_TRUE(str == fs::path(u32str.begin(), u32str.end()));
}
