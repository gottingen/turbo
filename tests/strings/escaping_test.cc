// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <turbo/strings/escaping.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <turbo/log/check.h>
#include <turbo/strings/str_cat.h>

#include <turbo/strings/internal/escaping_test_common.h>
#include <turbo/strings/string_view.h>

namespace {

    struct epair {
        std::string escaped;
        std::string unescaped;
    };

    TEST(c_encode, EscapeAndUnescape) {
        const std::string inputs[] = {
                std::string("foo\nxx\r\b\0023"),
                std::string(""),
                std::string("abc"),
                std::string("\1chad_rules"),
                std::string("\1arnar_drools"),
                std::string("xxxx\r\t'\"\\"),
                std::string("\0xx\0", 4),
                std::string("\x01\x31"),
                std::string("abc\xb\x42\141bc"),
                std::string("123\1\x31\x32\x33"),
                std::string("\xc1\xca\x1b\x62\x19o\xcc\x04"),
                std::string(
                        "\\\"\xe8\xb0\xb7\xe6\xad\x8c\\\" is Google\\\'s Chinese name"),
        };
        // Do this twice, once for octal escapes and once for hex escapes.
        for (int kind = 0; kind < 4; kind++) {
            for (const std::string &original: inputs) {
                std::string escaped;
                switch (kind) {
                    case 0:
                        escaped = turbo::c_encode(original);
                        break;
                    case 1:
                        escaped = turbo::c_hex_encode(original);
                        break;
                    case 2:
                        escaped = turbo::utf8_safe_encode(original);
                        break;
                    case 3:
                        escaped = turbo::utf8_safe_hex_encode(original);
                        break;
                }
                std::string unescaped_str;
                EXPECT_TRUE(turbo::c_decode(escaped, &unescaped_str));
                EXPECT_EQ(unescaped_str, original);

                unescaped_str.erase();
                std::string error;
                EXPECT_TRUE(turbo::c_decode(escaped, &unescaped_str, &error));
                EXPECT_EQ(error, "");

                // Check in-place unescaping
                std::string s = escaped;
                EXPECT_TRUE(turbo::c_decode(s, &s));
                ASSERT_EQ(s, original);
            }
        }
        // Check that all possible two character strings can be escaped then
        // unescaped successfully.
        for (int char0 = 0; char0 < 256; char0++) {
            for (int char1 = 0; char1 < 256; char1++) {
                char chars[2];
                chars[0] = char0;
                chars[1] = char1;
                std::string s(chars, 2);
                std::string escaped = turbo::c_hex_encode(s);
                std::string unescaped;
                EXPECT_TRUE(turbo::c_decode(escaped, &unescaped));
                EXPECT_EQ(s, unescaped);
            }
        }
    }

    TEST(c_encode, BasicEscaping) {
        epair oct_values[] = {
                {"foo\\rbar\\nbaz\\t",             "foo\rbar\nbaz\t"},
                {"\\'full of \\\"sound\\\" and \\\"fury\\\"\\'",
                                                   "'full of \"sound\" and \"fury\"'"},
                {"signi\\\\fying\\\\ nothing\\\\", "signi\\fying\\ nothing\\"},
                {"\\010\\t\\n\\013\\014\\r",       "\010\011\012\013\014\015"}
        };
        epair hex_values[] = {
                {"ubik\\rubik\\nubik\\t",         "ubik\rubik\nubik\t"},
                {"I\\\'ve just seen a \\\"face\\\"",
                                                  "I've just seen a \"face\""},
                {"hel\\\\ter\\\\skel\\\\ter\\\\", "hel\\ter\\skel\\ter\\"},
                {"\\x08\\t\\n\\x0b\\x0c\\r",      "\010\011\012\013\014\015"}
        };
        epair utf8_oct_values[] = {
                {"\xe8\xb0\xb7\xe6\xad\x8c\\r\xe8\xb0\xb7\xe6\xad\x8c\\nbaz\\t",
                        "\xe8\xb0\xb7\xe6\xad\x8c\r\xe8\xb0\xb7\xe6\xad\x8c\nbaz\t"},
                {"\\\"\xe8\xb0\xb7\xe6\xad\x8c\\\" is Google\\\'s Chinese name",
                        "\"\xe8\xb0\xb7\xe6\xad\x8c\" is Google\'s Chinese name"},
                {"\xe3\x83\xa1\xe3\x83\xbc\xe3\x83\xab\\\\are\\\\Japanese\\\\chars\\\\",
                        "\xe3\x83\xa1\xe3\x83\xbc\xe3\x83\xab\\are\\Japanese\\chars\\"},
                {"\xed\x81\xac\xeb\xa1\xac\\010\\t\\n\\013\\014\\r",
                        "\xed\x81\xac\xeb\xa1\xac\010\011\012\013\014\015"}
        };
        epair utf8_hex_values[] = {
                {"\x20\xe4\xbd\xa0\\t\xe5\xa5\xbd,\\r!\\n",
                        "\x20\xe4\xbd\xa0\t\xe5\xa5\xbd,\r!\n"},
                {"\xe8\xa9\xa6\xe9\xa8\x93\\\' means \\\"test\\\"",
                        "\xe8\xa9\xa6\xe9\xa8\x93\' means \"test\""},
                {"\\\\\xe6\x88\x91\\\\:\\\\\xe6\x9d\xa8\xe6\xac\xa2\\\\",
                        "\\\xe6\x88\x91\\:\\\xe6\x9d\xa8\xe6\xac\xa2\\"},
                {"\xed\x81\xac\xeb\xa1\xac\\x08\\t\\n\\x0b\\x0c\\r",
                        "\xed\x81\xac\xeb\xa1\xac\010\011\012\013\014\015"}
        };

        for (const epair &val: oct_values) {
            std::string escaped = turbo::c_encode(val.unescaped);
            EXPECT_EQ(escaped, val.escaped);
        }
        for (const epair &val: hex_values) {
            std::string escaped = turbo::c_hex_encode(val.unescaped);
            EXPECT_EQ(escaped, val.escaped);
        }
        for (const epair &val: utf8_oct_values) {
            std::string escaped = turbo::utf8_safe_encode(val.unescaped);
            EXPECT_EQ(escaped, val.escaped);
        }
        for (const epair &val: utf8_hex_values) {
            std::string escaped = turbo::utf8_safe_hex_encode(val.unescaped);
            EXPECT_EQ(escaped, val.escaped);
        }
    }

    TEST(Unescape, BasicFunction) {
        epair tests[] =
                {{"",            ""},
                 {"\\u0030",     "0"},
                 {"\\u00A3",     "\xC2\xA3"},
                 {"\\u22FD",     "\xE2\x8B\xBD"},
                 {"\\U00010000", "\xF0\x90\x80\x80"},
                 {"\\U0010FFFD", "\xF4\x8F\xBF\xBD"}};
        for (const epair &val: tests) {
            std::string out;
            EXPECT_TRUE(turbo::c_decode(val.escaped, &out));
            EXPECT_EQ(out, val.unescaped);
        }
        std::string bad[] = {"\\u1",         // too short
                             "\\U1",         // too short
                             "\\Uffffff",    // exceeds 0x10ffff (largest Unicode)
                             "\\U00110000",  // exceeds 0x10ffff (largest Unicode)
                             "\\uD835",      // surrogate character (D800-DFFF)
                             "\\U0000DD04",  // surrogate character (D800-DFFF)
                             "\\777",        // exceeds 0xff
                             "\\xABCD"};     // exceeds 0xff
        for (const std::string &e: bad) {
            std::string error;
            std::string out;
            EXPECT_FALSE(turbo::c_decode(e, &out, &error));
            EXPECT_FALSE(error.empty());

            out.erase();
            EXPECT_FALSE(turbo::c_decode(e, &out));
        }
    }

    class CUnescapeTest : public testing::Test {
    protected:
        static const char kStringWithMultipleOctalNulls[];
        static const char kStringWithMultipleHexNulls[];
        static const char kStringWithMultipleUnicodeNulls[];

        std::string result_string_;
    };

    const char CUnescapeTest::kStringWithMultipleOctalNulls[] =
            "\\0\\n"    // null escape \0 plus newline
            "0\\n"      // just a number 0 (not a null escape) plus newline
            "\\00\\12"  // null escape \00 plus octal newline code
            "\\000";    // null escape \000

// This has the same ingredients as kStringWithMultipleOctalNulls
// but with \x hex escapes instead of octal escapes.
    const char CUnescapeTest::kStringWithMultipleHexNulls[] =
            "\\x0\\n"
            "0\\n"
            "\\x00\\xa"
            "\\x000";

    const char CUnescapeTest::kStringWithMultipleUnicodeNulls[] =
            "\\u0000\\n"    // short-form (4-digit) null escape plus newline
            "0\\n"          // just a number 0 (not a null escape) plus newline
            "\\U00000000";  // long-form (8-digit) null escape

    TEST_F(CUnescapeTest, Unescapes1CharOctalNull) {
        std::string original_string = "\\0";
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0", 1), result_string_);
    }

    TEST_F(CUnescapeTest, Unescapes2CharOctalNull) {
        std::string original_string = "\\00";
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0", 1), result_string_);
    }

    TEST_F(CUnescapeTest, Unescapes3CharOctalNull) {
        std::string original_string = "\\000";
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0", 1), result_string_);
    }

    TEST_F(CUnescapeTest, Unescapes1CharHexNull) {
        std::string original_string = "\\x0";
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0", 1), result_string_);
    }

    TEST_F(CUnescapeTest, Unescapes2CharHexNull) {
        std::string original_string = "\\x00";
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0", 1), result_string_);
    }

    TEST_F(CUnescapeTest, Unescapes3CharHexNull) {
        std::string original_string = "\\x000";
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0", 1), result_string_);
    }

    TEST_F(CUnescapeTest, Unescapes4CharUnicodeNull) {
        std::string original_string = "\\u0000";
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0", 1), result_string_);
    }

    TEST_F(CUnescapeTest, Unescapes8CharUnicodeNull) {
        std::string original_string = "\\U00000000";
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0", 1), result_string_);
    }

    TEST_F(CUnescapeTest, UnescapesMultipleOctalNulls) {
        std::string original_string(kStringWithMultipleOctalNulls);
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        // All escapes, including newlines and null escapes, should have been
        // converted to the equivalent characters.
        EXPECT_EQ(std::string("\0\n"
                              "0\n"
                              "\0\n"
                              "\0",
                              7),
                  result_string_);
    }


    TEST_F(CUnescapeTest, UnescapesMultipleHexNulls) {
        std::string original_string(kStringWithMultipleHexNulls);
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0\n"
                              "0\n"
                              "\0\n"
                              "\0",
                              7),
                  result_string_);
    }

    TEST_F(CUnescapeTest, UnescapesMultipleUnicodeNulls) {
        std::string original_string(kStringWithMultipleUnicodeNulls);
        EXPECT_TRUE(turbo::c_decode(original_string, &result_string_));
        EXPECT_EQ(std::string("\0\n"
                              "0\n"
                              "\0",
                              5),
                  result_string_);
    }

    static struct {
        turbo::string_view plaintext;
        turbo::string_view cyphertext;
    } const base64_tests[] = {
            // Empty string.
            {{"",             0},          {"", 0}},
            {{nullptr,        0},
                                           {"", 0}},  // if length is zero, plaintext ptr must be ignored!

            // Basic bit patterns;
            // values obtained with "echo -n '...' | uuencode -m test"

            {{"\000",         1},          "AA=="},
            {{"\001",         1},          "AQ=="},
            {{"\002",         1},          "Ag=="},
            {{"\004",         1},          "BA=="},
            {{"\010",         1},          "CA=="},
            {{"\020",         1},          "EA=="},
            {{"\040",         1},          "IA=="},
            {{"\100",         1},          "QA=="},
            {{"\200",         1},          "gA=="},

            {{"\377",         1},          "/w=="},
            {{"\376",         1},          "/g=="},
            {{"\375",         1},          "/Q=="},
            {{"\373",         1},          "+w=="},
            {{"\367",         1},          "9w=="},
            {{"\357",         1},          "7w=="},
            {{"\337",         1},          "3w=="},
            {{"\277",         1},          "vw=="},
            {{"\177",         1},          "fw=="},
            {{"\000\000",     2},          "AAA="},
            {{"\000\001",     2},          "AAE="},
            {{"\000\002",     2},          "AAI="},
            {{"\000\004",     2},          "AAQ="},
            {{"\000\010",     2},          "AAg="},
            {{"\000\020",     2},          "ABA="},
            {{"\000\040",     2},          "ACA="},
            {{"\000\100",     2},          "AEA="},
            {{"\000\200",     2},          "AIA="},
            {{"\001\000",     2},          "AQA="},
            {{"\002\000",     2},          "AgA="},
            {{"\004\000",     2},          "BAA="},
            {{"\010\000",     2},          "CAA="},
            {{"\020\000",     2},          "EAA="},
            {{"\040\000",     2},          "IAA="},
            {{"\100\000",     2},          "QAA="},
            {{"\200\000",     2},          "gAA="},

            {{"\377\377",     2},          "//8="},
            {{"\377\376",     2},          "//4="},
            {{"\377\375",     2},          "//0="},
            {{"\377\373",     2},          "//s="},
            {{"\377\367",     2},          "//c="},
            {{"\377\357",     2},          "/+8="},
            {{"\377\337",     2},          "/98="},
            {{"\377\277",     2},          "/78="},
            {{"\377\177",     2},          "/38="},
            {{"\376\377",     2},          "/v8="},
            {{"\375\377",     2},          "/f8="},
            {{"\373\377",     2},          "+/8="},
            {{"\367\377",     2},          "9/8="},
            {{"\357\377",     2},          "7/8="},
            {{"\337\377",     2},          "3/8="},
            {{"\277\377",     2},          "v/8="},
            {{"\177\377",     2},          "f/8="},

            {{"\000\000\000", 3},          "AAAA"},
            {{"\000\000\001", 3},          "AAAB"},
            {{"\000\000\002", 3},          "AAAC"},
            {{"\000\000\004", 3},          "AAAE"},
            {{"\000\000\010", 3},          "AAAI"},
            {{"\000\000\020", 3},          "AAAQ"},
            {{"\000\000\040", 3},          "AAAg"},
            {{"\000\000\100", 3},          "AABA"},
            {{"\000\000\200", 3},          "AACA"},
            {{"\000\001\000", 3},          "AAEA"},
            {{"\000\002\000", 3},          "AAIA"},
            {{"\000\004\000", 3},          "AAQA"},
            {{"\000\010\000", 3},          "AAgA"},
            {{"\000\020\000", 3},          "ABAA"},
            {{"\000\040\000", 3},          "ACAA"},
            {{"\000\100\000", 3},          "AEAA"},
            {{"\000\200\000", 3},          "AIAA"},
            {{"\001\000\000", 3},          "AQAA"},
            {{"\002\000\000", 3},          "AgAA"},
            {{"\004\000\000", 3},          "BAAA"},
            {{"\010\000\000", 3},          "CAAA"},
            {{"\020\000\000", 3},          "EAAA"},
            {{"\040\000\000", 3},          "IAAA"},
            {{"\100\000\000", 3},          "QAAA"},
            {{"\200\000\000", 3},          "gAAA"},

            {{"\377\377\377", 3},          "////"},
            {{"\377\377\376", 3},          "///+"},
            {{"\377\377\375", 3},          "///9"},
            {{"\377\377\373", 3},          "///7"},
            {{"\377\377\367", 3},          "///3"},
            {{"\377\377\357", 3},          "///v"},
            {{"\377\377\337", 3},          "///f"},
            {{"\377\377\277", 3},          "//+/"},
            {{"\377\377\177", 3},          "//9/"},
            {{"\377\376\377", 3},          "//7/"},
            {{"\377\375\377", 3},          "//3/"},
            {{"\377\373\377", 3},          "//v/"},
            {{"\377\367\377", 3},          "//f/"},
            {{"\377\357\377", 3},          "/+//"},
            {{"\377\337\377", 3},          "/9//"},
            {{"\377\277\377", 3},          "/7//"},
            {{"\377\177\377", 3},          "/3//"},
            {{"\376\377\377", 3},          "/v//"},
            {{"\375\377\377", 3},          "/f//"},
            {{"\373\377\377", 3},          "+///"},
            {{"\367\377\377", 3},          "9///"},
            {{"\357\377\377", 3},          "7///"},
            {{"\337\377\377", 3},          "3///"},
            {{"\277\377\377", 3},          "v///"},
            {{"\177\377\377", 3},          "f///"},

            // Random numbers: values obtained with
            //
            //  #! /bin/bash
            //  dd bs=$1 count=1 if=/dev/random of=/tmp/bar.random
            //  od -N $1 -t o1 /tmp/bar.random
            //  uuencode -m test < /tmp/bar.random
            //
            // where $1 is the number of bytes (2, 3)

            {{"\243\361",     2},          "o/E="},
            {{"\024\167",     2},          "FHc="},
            {{"\313\252",     2},          "y6o="},
            {{"\046\041",     2},          "JiE="},
            {{"\145\236",     2},          "ZZ4="},
            {{"\254\325",     2},          "rNU="},
            {{"\061\330",     2},          "Mdg="},
            {{"\245\032",     2},          "pRo="},
            {{"\006\000",     2},          "BgA="},
            {{"\375\131",     2},          "/Vk="},
            {{"\303\210",     2},          "w4g="},
            {{"\040\037",     2},          "IB8="},
            {{"\261\372",     2},          "sfo="},
            {{"\335\014",     2},          "3Qw="},
            {{"\233\217",     2},          "m48="},
            {{"\373\056",     2},          "+y4="},
            {{"\247\232",     2},          "p5o="},
            {{"\107\053",     2},          "Rys="},
            {{"\204\077",     2},          "hD8="},
            {{"\276\211",     2},          "vok="},
            {{"\313\110",     2},          "y0g="},
            {{"\363\376",     2},          "8/4="},
            {{"\251\234",     2},          "qZw="},
            {{"\103\262",     2},          "Q7I="},
            {{"\142\312",     2},          "Yso="},
            {{"\067\211",     2},          "N4k="},
            {{"\220\001",     2},          "kAE="},
            {{"\152\240",     2},          "aqA="},
            {{"\367\061",     2},          "9zE="},
            {{"\133\255",     2},          "W60="},
            {{"\176\035",     2},          "fh0="},
            {{"\032\231",     2},          "Gpk="},

            {{"\013\007\144", 3},          "Cwdk"},
            {{"\030\112\106", 3},          "GEpG"},
            {{"\047\325\046", 3},          "J9Um"},
            {{"\310\160\022", 3},          "yHAS"},
            {{"\131\100\237", 3},          "WUCf"},
            {{"\064\342\134", 3},          "NOJc"},
            {{"\010\177\004", 3},          "CH8E"},
            {{"\345\147\205", 3},          "5WeF"},
            {{"\300\343\360", 3},          "wOPw"},
            {{"\061\240\201", 3},          "MaCB"},
            {{"\225\333\044", 3},          "ldsk"},
            {{"\215\137\352", 3},          "jV/q"},
            {{"\371\147\160", 3},          "+Wdw"},
            {{"\030\320\051", 3},          "GNAp"},
            {{"\044\174\241", 3},          "JHyh"},
            {{"\260\127\037", 3},          "sFcf"},
            {{"\111\045\033", 3},          "SSUb"},
            {{"\202\114\107", 3},          "gkxH"},
            {{"\057\371\042", 3},          "L/ki"},
            {{"\223\247\244", 3},          "k6ek"},
            {{"\047\216\144", 3},          "J45k"},
            {{"\203\070\327", 3},          "gzjX"},
            {{"\247\140\072", 3},          "p2A6"},
            {{"\124\115\116", 3},          "VE1O"},
            {{"\157\162\050", 3},          "b3Io"},
            {{"\357\223\004", 3},          "75ME"},
            {{"\052\117\156", 3},          "Kk9u"},
            {{"\347\154\000", 3},          "52wA"},
            {{"\303\012\142", 3},          "wwpi"},
            {{"\060\035\362", 3},          "MB3y"},
            {{"\130\226\361", 3},          "WJbx"},
            {{"\173\013\071", 3},          "ews5"},
            {{"\336\004\027", 3},          "3gQX"},
            {{"\357\366\234", 3},          "7/ac"},
            {{"\353\304\111", 3},          "68RJ"},
            {{"\024\264\131", 3},          "FLRZ"},
            {{"\075\114\251", 3},          "PUyp"},
            {{"\315\031\225", 3},          "zRmV"},
            {{"\154\201\276", 3},          "bIG+"},
            {{"\200\066\072", 3},          "gDY6"},
            {{"\142\350\267", 3},          "Yui3"},
            {{"\033\000\166", 3},          "GwB2"},
            {{"\210\055\077", 3},          "iC0/"},
            {{"\341\037\124", 3},          "4R9U"},
            {{"\161\103\152", 3},          "cUNq"},
            {{"\270\142\131", 3},          "uGJZ"},
            {{"\337\076\074", 3},          "3z48"},
            {{"\375\106\362", 3},          "/Uby"},
            {{"\227\301\127", 3},          "l8FX"},
            {{"\340\002\234", 3},          "4AKc"},
            {{"\121\064\033", 3},          "UTQb"},
            {{"\157\134\143", 3},          "b1xj"},
            {{"\247\055\327", 3},          "py3X"},
            {{"\340\142\005", 3},          "4GIF"},
            {{"\060\260\143", 3},          "MLBj"},
            {{"\075\203\170", 3},          "PYN4"},
            {{"\143\160\016", 3},          "Y3AO"},
            {{"\313\013\063", 3},          "ywsz"},
            {{"\174\236\135", 3},          "fJ5d"},
            {{"\103\047\026", 3},          "QycW"},
            {{"\365\005\343", 3},          "9QXj"},
            {{"\271\160\223", 3},          "uXCT"},
            {{"\362\255\172", 3},          "8q16"},
            {{"\113\012\015", 3},          "SwoN"},

            // various lengths, generated by this python script:
            //
            // from std::string import lowercase as lc
            // for i in range(27):
            //   print '{ %2d, "%s",%s "%s" },' % (i, lc[:i], ' ' * (26-i),
            //                                     lc[:i].encode('base64').strip())

            {{"",             0},          {"", 0}},
            {"a",                          "YQ=="},
            {"ab",                         "YWI="},
            {"abc",                        "YWJj"},
            {"abcd",                       "YWJjZA=="},
            {"abcde",                      "YWJjZGU="},
            {"abcdef",                     "YWJjZGVm"},
            {"abcdefg",                    "YWJjZGVmZw=="},
            {"abcdefgh",                   "YWJjZGVmZ2g="},
            {"abcdefghi",                  "YWJjZGVmZ2hp"},
            {"abcdefghij",                 "YWJjZGVmZ2hpag=="},
            {"abcdefghijk",                "YWJjZGVmZ2hpams="},
            {"abcdefghijkl",               "YWJjZGVmZ2hpamts"},
            {"abcdefghijklm",              "YWJjZGVmZ2hpamtsbQ=="},
            {"abcdefghijklmn",             "YWJjZGVmZ2hpamtsbW4="},
            {"abcdefghijklmno",            "YWJjZGVmZ2hpamtsbW5v"},
            {"abcdefghijklmnop",           "YWJjZGVmZ2hpamtsbW5vcA=="},
            {"abcdefghijklmnopq",          "YWJjZGVmZ2hpamtsbW5vcHE="},
            {"abcdefghijklmnopqr",         "YWJjZGVmZ2hpamtsbW5vcHFy"},
            {"abcdefghijklmnopqrs",        "YWJjZGVmZ2hpamtsbW5vcHFycw=="},
            {"abcdefghijklmnopqrst",       "YWJjZGVmZ2hpamtsbW5vcHFyc3Q="},
            {"abcdefghijklmnopqrstu",      "YWJjZGVmZ2hpamtsbW5vcHFyc3R1"},
            {"abcdefghijklmnopqrstuv",     "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dg=="},
            {"abcdefghijklmnopqrstuvw",    "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnc="},
            {"abcdefghijklmnopqrstuvwx",   "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4"},
            {"abcdefghijklmnopqrstuvwxy",  "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eQ=="},
            {"abcdefghijklmnopqrstuvwxyz", "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXo="},
    };

    template<typename StringType>
    void TestEscapeAndUnescape() {
        // Check the short strings; this tests the math (and boundaries)
        for (const auto &tc: base64_tests) {
            // Test plain base64.
            StringType encoded("this junk should be ignored");
            turbo::base64_encode(tc.plaintext, &encoded);
            EXPECT_EQ(encoded, tc.cyphertext);
            EXPECT_EQ(turbo::base64_encode(tc.plaintext), tc.cyphertext);

            StringType decoded("this junk should be ignored");
            EXPECT_TRUE(turbo::base64_decode(encoded, &decoded));
            EXPECT_EQ(decoded, tc.plaintext);

            StringType websafe_with_padding(tc.cyphertext);
            for (unsigned int c = 0; c < websafe_with_padding.size(); ++c) {
                if ('+' == websafe_with_padding[c]) websafe_with_padding[c] = '-';
                if ('/' == websafe_with_padding[c]) websafe_with_padding[c] = '_';
                // Intentionally keeping padding aka '='.
            }

            // Test plain websafe (aka without padding).
            StringType websafe(websafe_with_padding);
            for (unsigned int c = 0; c < websafe.size(); ++c) {
                if ('=' == websafe[c]) {
                    websafe.resize(c);
                    break;
                }
            }
            encoded = "this junk should be ignored";
            turbo::web_safe_base64_encode(tc.plaintext, &encoded);
            EXPECT_EQ(encoded, websafe);
            EXPECT_EQ(turbo::web_safe_base64_encode(tc.plaintext), websafe);

            decoded = "this junk should be ignored";
            EXPECT_TRUE(turbo::web_safe_base64_decode(websafe, &decoded));
            EXPECT_EQ(decoded, tc.plaintext);
        }

        // Now try the long strings, this tests the streaming
        for (const auto &tc: turbo::strings_internal::base64_strings()) {
            StringType buffer;
            turbo::web_safe_base64_encode(tc.plaintext, &buffer);
            EXPECT_EQ(tc.cyphertext, buffer);
            EXPECT_EQ(turbo::web_safe_base64_encode(tc.plaintext), tc.cyphertext);
        }

        // Verify the behavior when decoding bad data
        {
            turbo::string_view data_set[] = {"ab-/", turbo::string_view("\0bcd", 4),
                                             turbo::string_view("abc.\0", 5)};
            for (turbo::string_view bad_data: data_set) {
                StringType buf;
                EXPECT_FALSE(turbo::base64_decode(bad_data, &buf));
                EXPECT_FALSE(turbo::web_safe_base64_decode(bad_data, &buf));
                EXPECT_TRUE(buf.empty());
            }
        }
    }

    TEST(Base64, EscapeAndUnescape) {
        TestEscapeAndUnescape<std::string>();
    }

    TEST(Base64, Padding) {
        // Padding is optional.
        // '.' is an acceptable padding character, just like '='.
        std::initializer_list<turbo::string_view> good_padding = {
                "YQ",
                "YQ==",
                "YQ=.",
                "YQ.=",
                "YQ..",
        };
        for (turbo::string_view b64: good_padding) {
            std::string decoded;
            EXPECT_TRUE(turbo::base64_decode(b64, &decoded));
            EXPECT_EQ(decoded, "a");
            std::string websafe_decoded;
            EXPECT_TRUE(turbo::web_safe_base64_decode(b64, &websafe_decoded));
            EXPECT_EQ(websafe_decoded, "a");
        }
        std::initializer_list<turbo::string_view> bad_padding = {
                "YQ=",
                "YQ.",
                "YQ===",
                "YQ==.",
                "YQ=.=",
                "YQ=..",
                "YQ.==",
                "YQ.=.",
                "YQ..=",
                "YQ...",
                "YQ====",
                "YQ....",
                "YQ=====",
                "YQ.....",
        };
        for (turbo::string_view b64: bad_padding) {
            std::string decoded;
            EXPECT_FALSE(turbo::base64_decode(b64, &decoded));
            std::string websafe_decoded;
            EXPECT_FALSE(turbo::web_safe_base64_decode(b64, &websafe_decoded));
        }
    }

    TEST(Base64, DISABLED_HugeData) {
        const size_t kSize = size_t(3) * 1000 * 1000 * 1000;
        static_assert(kSize % 3 == 0, "kSize must be divisible by 3");
        const std::string huge(kSize, 'x');

        std::string escaped;
        turbo::base64_encode(huge, &escaped);

        // Generates the string that should match a base64 encoded "xxx..." string.
        // "xxx" in base64 is "eHh4".
        std::string expected_encoding;
        expected_encoding.reserve(kSize / 3 * 4);
        for (size_t i = 0; i < kSize / 3; ++i) {
            expected_encoding.append("eHh4");
        }
        EXPECT_EQ(expected_encoding, escaped);

        std::string unescaped;
        EXPECT_TRUE(turbo::base64_decode(escaped, &unescaped));
        EXPECT_EQ(huge, unescaped);
    }

    TEST(Escaping, HexStringToBytesBackToHex) {
        std::string bytes, hex;

        constexpr turbo::string_view kTestHexLower = "1c2f0032f40123456789abcdef";
        constexpr turbo::string_view kTestHexUpper = "1C2F0032F40123456789ABCDEF";
        constexpr turbo::string_view kTestBytes = turbo::string_view(
                "\x1c\x2f\x00\x32\xf4\x01\x23\x45\x67\x89\xab\xcd\xef", 13);

        EXPECT_TRUE(turbo::hex_string_to_bytes(kTestHexLower, &bytes));
        EXPECT_EQ(bytes, kTestBytes);

        EXPECT_TRUE(turbo::hex_string_to_bytes(kTestHexUpper, &bytes));
        EXPECT_EQ(bytes, kTestBytes);

        hex = turbo::bytes_to_hex_string(kTestBytes);
        EXPECT_EQ(hex, kTestHexLower);

        // Same buffer.
        // We do not care if this works since we do not promise it in the contract.
        // The purpose of this test is to to see if the program will crash or if
        // sanitizers will catch anything.
        bytes = std::string(kTestHexUpper);
        (void) turbo::hex_string_to_bytes(bytes, &bytes);

        // Length not a multiple of two.
        EXPECT_FALSE(turbo::hex_string_to_bytes("1c2f003", &bytes));

        // Not hex.
        EXPECT_FALSE(turbo::hex_string_to_bytes("1c2f00ft", &bytes));

        // Empty input.
        bytes = "abc";
        EXPECT_TRUE(turbo::hex_string_to_bytes("", &bytes));
        EXPECT_EQ("", bytes);  // Results in empty output.
    }

    TEST(HexAndBack, HexStringToBytes_and_BytesToHexString) {
        std::string hex_mixed = "0123456789abcdefABCDEF";
        std::string bytes_expected = "\x01\x23\x45\x67\x89\xab\xcd\xef\xAB\xCD\xEF";
        std::string hex_only_lower = "0123456789abcdefabcdef";

        std::string bytes_result;
        auto b = turbo::hex_string_to_bytes(hex_mixed, &bytes_result);
        EXPECT_EQ(b, true);
        EXPECT_EQ(bytes_expected, bytes_result);

        std::string prefix_valid = hex_mixed + "?";
        std::string prefix_valid_result;
        b = turbo::hex_string_to_bytes(
                turbo::string_view(prefix_valid.data(), prefix_valid.size() - 1), &prefix_valid_result);
        EXPECT_EQ(b, true);
        EXPECT_EQ(bytes_expected, prefix_valid_result);

        std::string infix_valid = "?" + hex_mixed + "???";
        std::string infix_valid_result;
        b = turbo::hex_string_to_bytes(
                turbo::string_view(infix_valid.data() + 1, hex_mixed.size()), &infix_valid_result);
        EXPECT_EQ(b, true);
        EXPECT_EQ(bytes_expected, infix_valid_result);

        std::string hex_result = turbo::bytes_to_hex_string(bytes_expected);
        EXPECT_EQ(hex_only_lower, hex_result);
    }

}  // namespace
