
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
// Unit tests for all join.h functions

#include "flare/strings/str_join.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <vector>

#include "testing/gtest_wrap.h"
#include "flare/base/profile.h"
#include "flare/strings/str_cat.h"
#include "flare/strings/str_split.h"

namespace {

    TEST(string_join, APIExamples) {
        {
            // Collection of strings
            std::vector<std::string> v = {"foo", "bar", "baz"};
            EXPECT_EQ("foo-bar-baz", flare::string_join(v, "-"));
        }

        {
            // Collection of std::string_view
            std::vector<std::string_view> v = {"foo", "bar", "baz"};
            EXPECT_EQ("foo-bar-baz", flare::string_join(v, "-"));
        }

        {
            // Collection of const char*
            std::vector<const char *> v = {"foo", "bar", "baz"};
            EXPECT_EQ("foo-bar-baz", flare::string_join(v, "-"));
        }

        {
            // Collection of non-const char*
            std::string a = "foo", b = "bar", c = "baz";
            std::vector<char *> v = {&a[0], &b[0], &c[0]};
            EXPECT_EQ("foo-bar-baz", flare::string_join(v, "-"));
        }

        {
            // Collection of ints
            std::vector<int> v = {1, 2, 3, -4};
            EXPECT_EQ("1-2-3--4", flare::string_join(v, "-"));
        }

        {
            // Literals passed as a std::initializer_list
            std::string s = flare::string_join({"a", "b", "c"}, "-");
            EXPECT_EQ("a-b-c", s);
        }
        {
            // Join a std::tuple<T...>.
            std::string s = flare::string_join(std::make_tuple(123, "abc", 0.456), "-");
            EXPECT_EQ("123-abc-0.456", s);
        }

        {
            // Collection of unique_ptrs
            std::vector<std::unique_ptr<int>> v;
            v.emplace_back(new int(1));
            v.emplace_back(new int(2));
            v.emplace_back(new int(3));
            EXPECT_EQ("1-2-3", flare::string_join(v, "-"));
        }

        {
            // Array of ints
            const int a[] = {1, 2, 3, -4};
            EXPECT_EQ("1-2-3--4", flare::string_join(a, a + FLARE_ARRAY_SIZE(a), "-"));
        }

        {
            // Collection of pointers
            int x = 1, y = 2, z = 3;
            std::vector<int *> v = {&x, &y, &z};
            EXPECT_EQ("1-2-3", flare::string_join(v, "-"));
        }

        {
            // Collection of pointers to pointers
            int x = 1, y = 2, z = 3;
            int *px = &x, *py = &y, *pz = &z;
            std::vector<int **> v = {&px, &py, &pz};
            EXPECT_EQ("1-2-3", flare::string_join(v, "-"));
        }

        {
            // Collection of pointers to std::string
            std::string a("a"), b("b");
            std::vector<std::string *> v = {&a, &b};
            EXPECT_EQ("a-b", flare::string_join(v, "-"));
        }

        {
            // A std::map, which is a collection of std::pair<>s.
            std::map<std::string, int> m = {{"a", 1},
                                            {"b", 2},
                                            {"c", 3}};
            EXPECT_EQ("a=1,b=2,c=3", flare::string_join(m, ",", flare::pair_formatter("=")));
        }

        {
            // Shows flare:: string_split and flare::string_join working together. This example is
            // equivalent to s/=/-/g.
            const std::string s = "a=b=c=d";
            EXPECT_EQ("a-b-c-d", flare::string_join(flare::string_split(s, "="), "-"));
        }

        //
        // A few examples of edge cases
        //

        {
            // Empty range yields an empty std::string.
            std::vector<std::string> v;
            EXPECT_EQ("", flare::string_join(v, "-"));
        }

        {
            // A range of 1 element gives a std::string with that element but no
            // separator.
            std::vector<std::string> v = {"foo"};
            EXPECT_EQ("foo", flare::string_join(v, "-"));
        }

        {
            // A range with a single empty std::string element
            std::vector<std::string> v = {""};
            EXPECT_EQ("", flare::string_join(v, "-"));
        }

        {
            // A range with 2 elements, one of which is an empty std::string
            std::vector<std::string> v = {"a", ""};
            EXPECT_EQ("a-", flare::string_join(v, "-"));
        }

        {
            // A range with 2 empty elements.
            std::vector<std::string> v = {"", ""};
            EXPECT_EQ("-", flare::string_join(v, "-"));
        }

        {
            // A std::vector of bool.
            std::vector<bool> v = {true, false, true};
            EXPECT_EQ("1-0-1", flare::string_join(v, "-"));
        }
    }

    TEST(string_join, CustomFormatter) {
        std::vector<std::string> v{"One", "Two", "Three"};
        {
            std::string joined =
                    flare::string_join(v, "", [](std::string *out, const std::string &in) {
                        flare::string_append(out, "(", in, ")");
                    });
            EXPECT_EQ("(One)(Two)(Three)", joined);
        }
        {
            class ImmovableFormatter {
            public:
                void operator()(std::string *out, const std::string &in) {
                    flare::string_append(out, "(", in, ")");
                }

                ImmovableFormatter() {}

                ImmovableFormatter(const ImmovableFormatter &) = delete;
            };
            EXPECT_EQ("(One)(Two)(Three)", flare::string_join(v, "", ImmovableFormatter()));
        }
        {
            class OverloadedFormatter {
            public:
                void operator()(std::string *out, const std::string &in) {
                    flare::string_append(out, "(", in, ")");
                }

                void operator()(std::string *out, const std::string &in) const {
                    flare::string_append(out, "[", in, "]");
                }
            };
            EXPECT_EQ("(One)(Two)(Three)", flare::string_join(v, "", OverloadedFormatter()));
            const OverloadedFormatter fmt = {};
            EXPECT_EQ("[One][Two][Three]", flare::string_join(v, "", fmt));
        }
    }

//
// Tests the Formatters
//

    TEST(alpha_num_formatter, FormatterAPI) {
        // Not an exhaustive test. See strings/strcat_test.h for the exhaustive test
        // of what alpha_num can convert.
        auto f = flare::alpha_num_formatter();
        std::string s;
        f(&s, "Testing: ");
        f(&s, static_cast<int>(1));
        f(&s, static_cast<int16_t>(2));
        f(&s, static_cast<int64_t>(3));
        f(&s, static_cast<float>(4));
        f(&s, static_cast<double>(5));
        f(&s, static_cast<unsigned>(6));
        f(&s, static_cast<size_t>(7));
        f(&s, std::string_view(" OK"));
        EXPECT_EQ("Testing: 1234567 OK", s);
    }

// Make sure people who are mistakenly using std::vector<bool> even though
// they're not memory-constrained can use flare::alpha_num_formatter().
    TEST(alpha_num_formatter, VectorOfBool) {
        auto f = flare::alpha_num_formatter();
        std::string s;
        std::vector<bool> v = {true, false, true};
        f(&s, *v.cbegin());
        f(&s, *v.begin());
        f(&s, v[1]);
        EXPECT_EQ("110", s);
    }

    TEST(alpha_num_formatter, alpha_num) {
        auto f = flare::alpha_num_formatter();
        std::string s;
        f(&s, flare::alpha_num("hello"));
        EXPECT_EQ("hello", s);
    }

    struct StreamableType {
        std::string contents;
    };

    inline std::ostream &operator<<(std::ostream &os, const StreamableType &t) {
        os << "Streamable:" << t.contents;
        return os;
    }

    TEST(stream_formatter, FormatterAPI) {
        auto f = flare::stream_formatter();
        std::string s;
        f(&s, "Testing: ");
        f(&s, static_cast<int>(1));
        f(&s, static_cast<int16_t>(2));
        f(&s, static_cast<int64_t>(3));
        f(&s, static_cast<float>(4));
        f(&s, static_cast<double>(5));
        f(&s, static_cast<unsigned>(6));
        f(&s, static_cast<size_t>(7));
        f(&s, std::string_view(" OK "));
        StreamableType streamable = {"object"};
        f(&s, streamable);
        EXPECT_EQ("Testing: 1234567 OK Streamable:object", s);
    }

// A dummy formatter that wraps each element in parens. Used in some tests
// below.
    struct TestingParenFormatter {
        template<typename T>
        void operator()(std::string *s, const T &t) {
            flare::string_append(s, "(", t, ")");
        }
    };

    TEST(pair_formatter, FormatterAPI) {
        {
            // Tests default pair_formatter(sep) that uses alpha_num_formatter for the
            // 'first' and 'second' members.
            const auto f = flare::pair_formatter("=");
            std::string s;
            f(&s, std::make_pair("a", "b"));
            f(&s, std::make_pair(1, 2));
            EXPECT_EQ("a=b1=2", s);
        }

        {
            // Tests using a custom formatter for the 'first' and 'second' members.
            auto f = flare::pair_formatter(TestingParenFormatter(), "=",
                                          TestingParenFormatter());
            std::string s;
            f(&s, std::make_pair("a", "b"));
            f(&s, std::make_pair(1, 2));
            EXPECT_EQ("(a)=(b)(1)=(2)", s);
        }
    }

    TEST(dereference_formatter, FormatterAPI) {
        {
            // Tests wrapping the default alpha_num_formatter.
            const flare::strings_internal::dereference_formatter_impl<
                    flare::strings_internal::alpha_num_formatter_impl>
                    f;
            int x = 1, y = 2, z = 3;
            std::string s;
            f(&s, &x);
            f(&s, &y);
            f(&s, &z);
            EXPECT_EQ("123", s);
        }

        {
            // Tests wrapping std::string's default formatter.
            flare::strings_internal::dereference_formatter_impl<
                    flare::strings_internal::default_formatter<std::string>::Type>
                    f;

            std::string x = "x";
            std::string y = "y";
            std::string z = "z";
            std::string s;
            f(&s, &x);
            f(&s, &y);
            f(&s, &z);
            EXPECT_EQ(s, "xyz");
        }

        {
            // Tests wrapping a custom formatter.
            auto f = flare::dereference_formatter(TestingParenFormatter());
            int x = 1, y = 2, z = 3;
            std::string s;
            f(&s, &x);
            f(&s, &y);
            f(&s, &z);
            EXPECT_EQ("(1)(2)(3)", s);
        }

        {
            flare::strings_internal::dereference_formatter_impl<
                    flare::strings_internal::alpha_num_formatter_impl>
                    f;
            auto x = std::unique_ptr<int>(new int(1));
            auto y = std::unique_ptr<int>(new int(2));
            auto z = std::unique_ptr<int>(new int(3));
            std::string s;
            f(&s, x);
            f(&s, y);
            f(&s, z);
            EXPECT_EQ("123", s);
        }
    }

//
// Tests the interfaces for the 4 public Join function overloads. The semantics
// of the algorithm is covered in the above APIExamples test.
//
    TEST(string_join, PublicAPIOverloads) {
        std::vector<std::string> v = {"a", "b", "c"};

        // Iterators + formatter
        EXPECT_EQ("a-b-c",
                  flare::string_join(v.begin(), v.end(), "-", flare::alpha_num_formatter()));
        // Range + formatter
        EXPECT_EQ("a-b-c", flare::string_join(v, "-", flare::alpha_num_formatter()));
        // Iterators, no formatter
        EXPECT_EQ("a-b-c", flare::string_join(v.begin(), v.end(), "-"));
        // Range, no formatter
        EXPECT_EQ("a-b-c", flare::string_join(v, "-"));
    }

    TEST(string_join, Array) {
        const std::string_view a[] = {"a", "b", "c"};
        EXPECT_EQ("a-b-c", flare::string_join(a, "-"));
    }

    TEST(string_join, InitializerList) {
        { EXPECT_EQ("a-b-c", flare::string_join({"a", "b", "c"}, "-")); }

        {
            auto a = {"a", "b", "c"};
            EXPECT_EQ("a-b-c", flare::string_join(a, "-"));
        }

        {
            std::initializer_list<const char *> a = {"a", "b", "c"};
            EXPECT_EQ("a-b-c", flare::string_join(a, "-"));
        }

        {
            std::initializer_list<std::string> a = {"a", "b", "c"};
            EXPECT_EQ("a-b-c", flare::string_join(a, "-"));
        }

        {
            std::initializer_list<std::string_view> a = {"a", "b", "c"};
            EXPECT_EQ("a-b-c", flare::string_join(a, "-"));
        }

        {
            // Tests initializer_list with a non-default formatter
            auto a = {"a", "b", "c"};
            TestingParenFormatter f;
            EXPECT_EQ("(a)-(b)-(c)", flare::string_join(a, "-", f));
        }

        {
            // initializer_list of ints
            EXPECT_EQ("1-2-3", flare::string_join({1, 2, 3}, "-"));
        }

        {
            // Tests initializer_list of ints with a non-default formatter
            auto a = {1, 2, 3};
            TestingParenFormatter f;
            EXPECT_EQ("(1)-(2)-(3)", flare::string_join(a, "-", f));
        }
    }

    TEST(string_join, Tuple) {
        EXPECT_EQ("", flare::string_join(std::make_tuple(), "-"));
        EXPECT_EQ("hello", flare::string_join(std::make_tuple("hello"), "-"));

        int x(10);
        std::string y("hello");
        double z(3.14);
        EXPECT_EQ("10-hello-3.14", flare::string_join(std::make_tuple(x, y, z), "-"));

        // Faster! Faster!!
        EXPECT_EQ("10-hello-3.14",
                  flare::string_join(std::make_tuple(x, std::cref(y), z), "-"));

        struct TestFormatter {
            char buffer[128];

            void operator()(std::string *out, int v) {
                snprintf(buffer, sizeof(buffer), "%#.8x", v);
                out->append(buffer);
            }

            void operator()(std::string *out, double v) {
                snprintf(buffer, sizeof(buffer), "%#.0f", v);
                out->append(buffer);
            }

            void operator()(std::string *out, const std::string &v) {
                snprintf(buffer, sizeof(buffer), "%.4s", v.c_str());
                out->append(buffer);
            }
        };
        EXPECT_EQ("0x0000000a-hell-3.",
                  flare::string_join(std::make_tuple(x, y, z), "-", TestFormatter()));
        EXPECT_EQ(
                "0x0000000a-hell-3.",
                flare::string_join(std::make_tuple(x, std::cref(y), z), "-", TestFormatter()));
        EXPECT_EQ("0x0000000a-hell-3.",
                  flare::string_join(std::make_tuple(&x, &y, &z), "-",
                                    flare::dereference_formatter(TestFormatter())));
        EXPECT_EQ("0x0000000a-hell-3.",
                  flare::string_join(std::make_tuple(std::make_unique<int>(x),
                                                    std::make_unique<std::string>(y),
                                                    std::make_unique<double>(z)),
                                    "-", flare::dereference_formatter(TestFormatter())));
        EXPECT_EQ("0x0000000a-hell-3.",
                  flare::string_join(std::make_tuple(std::make_unique<int>(x), &y, &z),
                                    "-", flare::dereference_formatter(TestFormatter())));
    }

}  // namespace
