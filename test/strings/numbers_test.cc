
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
// This file tests string processing functions related to numeric values.

#include "flare/strings/numbers.h"
#include "flare/strings/internal/ostringstream.h"
#include <sys/types.h>
#include "flare/base/fast_rand.h"
#include <cfenv>  // NOLINT(build/c++11)
#include <cinttypes>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>


#include "testing/gtest_wrap.h"
#include "flare/log/logging.h"
#include "numbers_test_common.h"
#include "flare/strings/str_cat.h"


namespace {

// The exact value of 1e23 falls precisely halfway between two representable
// doubles. Furthermore, the rounding rules we prefer (break ties by rounding
// to the nearest even) dictate in this case that the number should be rounded
// down, but this is not completely specified for floating-point literals in
// C++. (It just says to use the default rounding mode of the standard
// library.) We ensure the result we want by using a number that has an
// unambiguous correctly rounded answer.
    constexpr double k1e23 = 9999999999999999e7;

    constexpr double kPowersOfTen[] = {
            0.0, 1e-323, 1e-322, 1e-321, 1e-320, 1e-319, 1e-318, 1e-317, 1e-316,
            1e-315, 1e-314, 1e-313, 1e-312, 1e-311, 1e-310, 1e-309, 1e-308, 1e-307,
            1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300, 1e-299, 1e-298,
            1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292, 1e-291, 1e-290, 1e-289,
            1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282, 1e-281, 1e-280,
            1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274, 1e-273, 1e-272, 1e-271,
            1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264, 1e-263, 1e-262,
            1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256, 1e-255, 1e-254, 1e-253,
            1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246, 1e-245, 1e-244,
            1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238, 1e-237, 1e-236, 1e-235,
            1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228, 1e-227, 1e-226,
            1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220, 1e-219, 1e-218, 1e-217,
            1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210, 1e-209, 1e-208,
            1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202, 1e-201, 1e-200, 1e-199,
            1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192, 1e-191, 1e-190,
            1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184, 1e-183, 1e-182, 1e-181,
            1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174, 1e-173, 1e-172,
            1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166, 1e-165, 1e-164, 1e-163,
            1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156, 1e-155, 1e-154,
            1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148, 1e-147, 1e-146, 1e-145,
            1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138, 1e-137, 1e-136,
            1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130, 1e-129, 1e-128, 1e-127,
            1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120, 1e-119, 1e-118,
            1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112, 1e-111, 1e-110, 1e-109,
            1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102, 1e-101, 1e-100,
            1e-99, 1e-98, 1e-97, 1e-96, 1e-95, 1e-94, 1e-93, 1e-92, 1e-91,
            1e-90, 1e-89, 1e-88, 1e-87, 1e-86, 1e-85, 1e-84, 1e-83, 1e-82,
            1e-81, 1e-80, 1e-79, 1e-78, 1e-77, 1e-76, 1e-75, 1e-74, 1e-73,
            1e-72, 1e-71, 1e-70, 1e-69, 1e-68, 1e-67, 1e-66, 1e-65, 1e-64,
            1e-63, 1e-62, 1e-61, 1e-60, 1e-59, 1e-58, 1e-57, 1e-56, 1e-55,
            1e-54, 1e-53, 1e-52, 1e-51, 1e-50, 1e-49, 1e-48, 1e-47, 1e-46,
            1e-45, 1e-44, 1e-43, 1e-42, 1e-41, 1e-40, 1e-39, 1e-38, 1e-37,
            1e-36, 1e-35, 1e-34, 1e-33, 1e-32, 1e-31, 1e-30, 1e-29, 1e-28,
            1e-27, 1e-26, 1e-25, 1e-24, 1e-23, 1e-22, 1e-21, 1e-20, 1e-19,
            1e-18, 1e-17, 1e-16, 1e-15, 1e-14, 1e-13, 1e-12, 1e-11, 1e-10,
            1e-9, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 1e-3, 1e-2, 1e-1,
            1e+0, 1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8,
            1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17,
            1e+18, 1e+19, 1e+20, 1e+21, 1e+22, k1e23, 1e+24, 1e+25, 1e+26,
            1e+27, 1e+28, 1e+29, 1e+30, 1e+31, 1e+32, 1e+33, 1e+34, 1e+35,
            1e+36, 1e+37, 1e+38, 1e+39, 1e+40, 1e+41, 1e+42, 1e+43, 1e+44,
            1e+45, 1e+46, 1e+47, 1e+48, 1e+49, 1e+50, 1e+51, 1e+52, 1e+53,
            1e+54, 1e+55, 1e+56, 1e+57, 1e+58, 1e+59, 1e+60, 1e+61, 1e+62,
            1e+63, 1e+64, 1e+65, 1e+66, 1e+67, 1e+68, 1e+69, 1e+70, 1e+71,
            1e+72, 1e+73, 1e+74, 1e+75, 1e+76, 1e+77, 1e+78, 1e+79, 1e+80,
            1e+81, 1e+82, 1e+83, 1e+84, 1e+85, 1e+86, 1e+87, 1e+88, 1e+89,
            1e+90, 1e+91, 1e+92, 1e+93, 1e+94, 1e+95, 1e+96, 1e+97, 1e+98,
            1e+99, 1e+100, 1e+101, 1e+102, 1e+103, 1e+104, 1e+105, 1e+106, 1e+107,
            1e+108, 1e+109, 1e+110, 1e+111, 1e+112, 1e+113, 1e+114, 1e+115, 1e+116,
            1e+117, 1e+118, 1e+119, 1e+120, 1e+121, 1e+122, 1e+123, 1e+124, 1e+125,
            1e+126, 1e+127, 1e+128, 1e+129, 1e+130, 1e+131, 1e+132, 1e+133, 1e+134,
            1e+135, 1e+136, 1e+137, 1e+138, 1e+139, 1e+140, 1e+141, 1e+142, 1e+143,
            1e+144, 1e+145, 1e+146, 1e+147, 1e+148, 1e+149, 1e+150, 1e+151, 1e+152,
            1e+153, 1e+154, 1e+155, 1e+156, 1e+157, 1e+158, 1e+159, 1e+160, 1e+161,
            1e+162, 1e+163, 1e+164, 1e+165, 1e+166, 1e+167, 1e+168, 1e+169, 1e+170,
            1e+171, 1e+172, 1e+173, 1e+174, 1e+175, 1e+176, 1e+177, 1e+178, 1e+179,
            1e+180, 1e+181, 1e+182, 1e+183, 1e+184, 1e+185, 1e+186, 1e+187, 1e+188,
            1e+189, 1e+190, 1e+191, 1e+192, 1e+193, 1e+194, 1e+195, 1e+196, 1e+197,
            1e+198, 1e+199, 1e+200, 1e+201, 1e+202, 1e+203, 1e+204, 1e+205, 1e+206,
            1e+207, 1e+208, 1e+209, 1e+210, 1e+211, 1e+212, 1e+213, 1e+214, 1e+215,
            1e+216, 1e+217, 1e+218, 1e+219, 1e+220, 1e+221, 1e+222, 1e+223, 1e+224,
            1e+225, 1e+226, 1e+227, 1e+228, 1e+229, 1e+230, 1e+231, 1e+232, 1e+233,
            1e+234, 1e+235, 1e+236, 1e+237, 1e+238, 1e+239, 1e+240, 1e+241, 1e+242,
            1e+243, 1e+244, 1e+245, 1e+246, 1e+247, 1e+248, 1e+249, 1e+250, 1e+251,
            1e+252, 1e+253, 1e+254, 1e+255, 1e+256, 1e+257, 1e+258, 1e+259, 1e+260,
            1e+261, 1e+262, 1e+263, 1e+264, 1e+265, 1e+266, 1e+267, 1e+268, 1e+269,
            1e+270, 1e+271, 1e+272, 1e+273, 1e+274, 1e+275, 1e+276, 1e+277, 1e+278,
            1e+279, 1e+280, 1e+281, 1e+282, 1e+283, 1e+284, 1e+285, 1e+286, 1e+287,
            1e+288, 1e+289, 1e+290, 1e+291, 1e+292, 1e+293, 1e+294, 1e+295, 1e+296,
            1e+297, 1e+298, 1e+299, 1e+300, 1e+301, 1e+302, 1e+303, 1e+304, 1e+305,
            1e+306, 1e+307, 1e+308,
    };

}  // namespace

double Pow10(int exp) {
    if (exp < -324) {
        return 0.0;
    } else if (exp > 308) {
        return INFINITY;
    } else {
        return kPowersOfTen[exp + 324];
    }
}


namespace {

    using flare::numbers_internal::kSixDigitsToBufferSize;
    using flare::numbers_internal::safe_strto32_base;
    using flare::numbers_internal::safe_strto64_base;
    using flare::numbers_internal::safe_strtou32_base;
    using flare::numbers_internal::safe_strtou64_base;
    using flare::numbers_internal::six_digits_to_buffer;
    using flare::strings_internal::Itoa;
    using flare::strings_internal::strtouint32_test_cases;
    using flare::strings_internal::strtouint64_test_cases;
    using flare::simple_atoi;
    using testing::Eq;
    using testing::MatchesRegex;

// Number of floats to test with.
// 5,000,000 is a reasonable default for a test that only takes a few seconds.
// 1,000,000,000+ triggers checking for all possible mantissa values for
// double-precision tests. 2,000,000,000+ triggers checking for every possible
// single-precision float.
    const int kFloatNumCases = 5000000;

// This is a slow, brute-force routine to compute the exact base-10
// representation of a double-precision floating-point number.  It
// is useful for debugging only.
    std::string PerfectDtoa(double d) {
        if (d == 0) return "0";
        if (d < 0) return "-" + PerfectDtoa(-d);

        // Basic theory: decompose d into mantissa and exp, where
        // d = mantissa * 2^exp, and exp is as close to zero as possible.
        int64_t mantissa, exp = 0;
        while (d >= 1ULL << 63) ++exp, d *= 0.5;
        while ((mantissa = d) != d) --exp, d *= 2.0;

        // Then convert mantissa to ASCII, and either double it (if
        // exp > 0) or halve it (if exp < 0) repeatedly.  "halve it"
        // in this case means multiplying it by five and dividing by 10.
        constexpr int maxlen = 1100;  // worst case is actually 1030 or so.
        char buf[maxlen + 5];
        for (int64_t num = mantissa, pos = maxlen; --pos >= 0;) {
            buf[pos] = '0' + (num % 10);
            num /= 10;
        }
        char *begin = &buf[0];
        char *end = buf + maxlen;
        for (int i = 0; i != exp; i += (exp > 0) ? 1 : -1) {
            int carry = 0;
            for (char *p = end; --p != begin;) {
                int dig = *p - '0';
                dig = dig * (exp > 0 ? 2 : 5) + carry;
                carry = dig / 10;
                dig %= 10;
                *p = '0' + dig;
            }
        }
        if (exp < 0) {
            // "dividing by 10" above means we have to add the decimal point.
            memmove(end + 1 + exp, end + exp, 1 - exp);
            end[exp] = '.';
            ++end;
        }
        while (*begin == '0' && begin[1] != '.') ++begin;
        return {begin, end};
    }

    TEST(ToString, PerfectDtoa) {
        EXPECT_THAT(PerfectDtoa(1), Eq("1"));
        EXPECT_THAT(PerfectDtoa(0.1),
                    Eq("0.1000000000000000055511151231257827021181583404541015625"));
        EXPECT_THAT(PerfectDtoa(1e24), Eq("999999999999999983222784"));
        EXPECT_THAT(PerfectDtoa(5e-324), MatchesRegex("0.0000.*625"));
        for (int i = 0; i < 100; ++i) {
            for (double multiplier :
                    {1e-300, 1e-200, 1e-100, 0.1, 1.0, 10.0, 1e100, 1e300}) {
                double d = multiplier * i;
                std::string s = PerfectDtoa(d);
                EXPECT_DOUBLE_EQ(d, strtod(s.c_str(), nullptr));
            }
        }
    }

    template<typename integer>
    struct MyInteger {
        integer i;

        explicit constexpr MyInteger(integer i) : i(i) {}

        constexpr operator integer() const { return i; }

        constexpr MyInteger operator+(MyInteger other) const { return i + other.i; }

        constexpr MyInteger operator-(MyInteger other) const { return i - other.i; }

        constexpr MyInteger operator*(MyInteger other) const { return i * other.i; }

        constexpr MyInteger operator/(MyInteger other) const { return i / other.i; }

        constexpr bool operator<(MyInteger other) const { return i < other.i; }

        constexpr bool operator<=(MyInteger other) const { return i <= other.i; }

        constexpr bool operator==(MyInteger other) const { return i == other.i; }

        constexpr bool operator>=(MyInteger other) const { return i >= other.i; }

        constexpr bool operator>(MyInteger other) const { return i > other.i; }

        constexpr bool operator!=(MyInteger other) const { return i != other.i; }

        integer as_integer() const { return i; }
    };

    typedef MyInteger<int64_t> MyInt64;
    typedef MyInteger<uint64_t> MyUInt64;

    void CheckInt32(int32_t x) {
        char buffer[flare::numbers_internal::kFastToBufferSize];
        char *actual = flare::numbers_internal::fast_int_to_buffer(x, buffer);
        std::string expected = std::to_string(x);
        EXPECT_EQ(expected, std::string(buffer, actual)) << " Input " << x;

        char *generic_actual = flare::numbers_internal::fast_int_to_buffer(x, buffer);
        EXPECT_EQ(expected, std::string(buffer, generic_actual)) << " Input " << x;
    }

    void CheckInt64(int64_t x) {
        char buffer[flare::numbers_internal::kFastToBufferSize + 3];
        buffer[0] = '*';
        buffer[23] = '*';
        buffer[24] = '*';
        char *actual = flare::numbers_internal::fast_int_to_buffer(x, &buffer[1]);
        std::string expected = std::to_string(x);
        EXPECT_EQ(expected, std::string(&buffer[1], actual)) << " Input " << x;
        EXPECT_EQ(buffer[0], '*');
        EXPECT_EQ(buffer[23], '*');
        EXPECT_EQ(buffer[24], '*');

        char *my_actual =
                flare::numbers_internal::fast_int_to_buffer(MyInt64(x), &buffer[1]);
        EXPECT_EQ(expected, std::string(&buffer[1], my_actual)) << " Input " << x;
    }

    void CheckUInt32(uint32_t x) {
        char buffer[flare::numbers_internal::kFastToBufferSize];
        char *actual = flare::numbers_internal::fast_int_to_buffer(x, buffer);
        std::string expected = std::to_string(x);
        EXPECT_EQ(expected, std::string(buffer, actual)) << " Input " << x;

        char *generic_actual = flare::numbers_internal::fast_int_to_buffer(x, buffer);
        EXPECT_EQ(expected, std::string(buffer, generic_actual)) << " Input " << x;
    }

    void CheckUInt64(uint64_t x) {
        char buffer[flare::numbers_internal::kFastToBufferSize + 1];
        char *actual = flare::numbers_internal::fast_int_to_buffer(x, &buffer[1]);
        std::string expected = std::to_string(x);
        EXPECT_EQ(expected, std::string(&buffer[1], actual)) << " Input " << x;

        char *generic_actual = flare::numbers_internal::fast_int_to_buffer(x, &buffer[1]);
        EXPECT_EQ(expected, std::string(&buffer[1], generic_actual))
                            << " Input " << x;

        char *my_actual =
                flare::numbers_internal::fast_int_to_buffer(MyUInt64(x), &buffer[1]);
        EXPECT_EQ(expected, std::string(&buffer[1], my_actual)) << " Input " << x;
    }

    void CheckHex64(uint64_t v) {
        char expected[16 + 1];
        std::string actual = flare::string_cat(flare::hex(v, flare::kZeroPad16));
        snprintf(expected, sizeof(expected), "%016" PRIx64, static_cast<uint64_t>(v));
        EXPECT_EQ(expected, actual) << " Input " << v;
        actual = flare::string_cat(flare::hex(v, flare::kSpacePad16));
        snprintf(expected, sizeof(expected), "%16" PRIx64, static_cast<uint64_t>(v));
        EXPECT_EQ(expected, actual) << " Input " << v;
    }

    TEST(Numbers, TestFastPrints) {
        for (int i = -100; i <= 100; i++) {
            CheckInt32(i);
            CheckInt64(i);
        }
        for (int i = 0; i <= 100; i++) {
            CheckUInt32(i);
            CheckUInt64(i);
        }
        // Test min int to make sure that works
        CheckInt32(INT_MIN);
        CheckInt32(INT_MAX);
        CheckInt64(LONG_MIN);
        CheckInt64(uint64_t{1000000000});
        CheckInt64(uint64_t{9999999999});
        CheckInt64(uint64_t{100000000000000});
        CheckInt64(uint64_t{999999999999999});
        CheckInt64(uint64_t{1000000000000000000});
        CheckInt64(uint64_t{1199999999999999999});
        CheckInt64(int64_t{-700000000000000000});
        CheckInt64(LONG_MAX);
        CheckUInt32(std::numeric_limits<uint32_t>::max());
        CheckUInt64(uint64_t{1000000000});
        CheckUInt64(uint64_t{9999999999});
        CheckUInt64(uint64_t{100000000000000});
        CheckUInt64(uint64_t{999999999999999});
        CheckUInt64(uint64_t{1000000000000000000});
        CheckUInt64(uint64_t{1199999999999999999});
        CheckUInt64(std::numeric_limits<uint64_t>::max());

        for (int i = 0; i < 10000; i++) {
            CheckHex64(i);
        }
        CheckHex64(uint64_t{0x123456789abcdef0});
    }

    template<typename int_type, typename in_val_type>
    void VerifySimpleAtoiGood(in_val_type in_value, int_type exp_value) {
        std::string s;
        // uint128 can be streamed but not string_cat'd
        flare::strings_internal::string_output_stream(&s) << in_value;
        int_type x = static_cast<int_type>(~exp_value);
        EXPECT_TRUE(simple_atoi(s, &x))
                            << "in_value=" << in_value << " s=" << s << " x=" << x;
        EXPECT_EQ(exp_value, x);
        x = static_cast<int_type>(~exp_value);
        EXPECT_TRUE(simple_atoi(s.c_str(), &x));
        EXPECT_EQ(exp_value, x);
    }

    template<typename int_type, typename in_val_type>
    void VerifySimpleAtoiBad(in_val_type in_value) {
        std::string s = flare::string_cat(in_value);
        int_type x;
        EXPECT_FALSE(simple_atoi(s, &x));
        EXPECT_FALSE(simple_atoi(s.c_str(), &x));
    }

    TEST(NumbersTest, Atoi) {
        // simple_atoi(std::string_view, int32_t)
        VerifySimpleAtoiGood<int32_t>(0, 0);
        VerifySimpleAtoiGood<int32_t>(42, 42);
        VerifySimpleAtoiGood<int32_t>(-42, -42);

        VerifySimpleAtoiGood<int32_t>(std::numeric_limits<int32_t>::min(),
                                      std::numeric_limits<int32_t>::min());
        VerifySimpleAtoiGood<int32_t>(std::numeric_limits<int32_t>::max(),
                                      std::numeric_limits<int32_t>::max());

        // simple_atoi(std::string_view, uint32_t)
        VerifySimpleAtoiGood<uint32_t>(0, 0);
        VerifySimpleAtoiGood<uint32_t>(42, 42);
        VerifySimpleAtoiBad<uint32_t>(-42);

        VerifySimpleAtoiBad<uint32_t>(std::numeric_limits<int32_t>::min());
        VerifySimpleAtoiGood<uint32_t>(std::numeric_limits<int32_t>::max(),
                                       std::numeric_limits<int32_t>::max());
        VerifySimpleAtoiGood<uint32_t>(std::numeric_limits<uint32_t>::max(),
                                       std::numeric_limits<uint32_t>::max());
        VerifySimpleAtoiBad<uint32_t>(std::numeric_limits<int64_t>::min());
        VerifySimpleAtoiBad<uint32_t>(std::numeric_limits<int64_t>::max());
        VerifySimpleAtoiBad<uint32_t>(std::numeric_limits<uint64_t>::max());

        // simple_atoi(std::string_view, int64_t)
        VerifySimpleAtoiGood<int64_t>(0, 0);
        VerifySimpleAtoiGood<int64_t>(42, 42);
        VerifySimpleAtoiGood<int64_t>(-42, -42);

        VerifySimpleAtoiGood<int64_t>(std::numeric_limits<int32_t>::min(),
                                      std::numeric_limits<int32_t>::min());
        VerifySimpleAtoiGood<int64_t>(std::numeric_limits<int32_t>::max(),
                                      std::numeric_limits<int32_t>::max());
        VerifySimpleAtoiGood<int64_t>(std::numeric_limits<uint32_t>::max(),
                                      std::numeric_limits<uint32_t>::max());
        VerifySimpleAtoiGood<int64_t>(std::numeric_limits<int64_t>::min(),
                                      std::numeric_limits<int64_t>::min());
        VerifySimpleAtoiGood<int64_t>(std::numeric_limits<int64_t>::max(),
                                      std::numeric_limits<int64_t>::max());
        VerifySimpleAtoiBad<int64_t>(std::numeric_limits<uint64_t>::max());

        // simple_atoi(std::string_view, uint64_t)
        VerifySimpleAtoiGood<uint64_t>(0, 0);
        VerifySimpleAtoiGood<uint64_t>(42, 42);
        VerifySimpleAtoiBad<uint64_t>(-42);

        VerifySimpleAtoiBad<uint64_t>(std::numeric_limits<int32_t>::min());
        VerifySimpleAtoiGood<uint64_t>(std::numeric_limits<int32_t>::max(),
                                       std::numeric_limits<int32_t>::max());
        VerifySimpleAtoiGood<uint64_t>(std::numeric_limits<uint32_t>::max(),
                                       std::numeric_limits<uint32_t>::max());
        VerifySimpleAtoiBad<uint64_t>(std::numeric_limits<int64_t>::min());
        VerifySimpleAtoiGood<uint64_t>(std::numeric_limits<int64_t>::max(),
                                       std::numeric_limits<int64_t>::max());
        VerifySimpleAtoiGood<uint64_t>(std::numeric_limits<uint64_t>::max(),
                                       std::numeric_limits<uint64_t>::max());

        // simple_atoi(std::string_view, flare::uint128)
        VerifySimpleAtoiGood<flare::uint128>(0, 0);
        VerifySimpleAtoiGood<flare::uint128>(42, 42);
        VerifySimpleAtoiBad<flare::uint128>(-42);

        VerifySimpleAtoiBad<flare::uint128>(std::numeric_limits<int32_t>::min());
        VerifySimpleAtoiGood<flare::uint128>(std::numeric_limits<int32_t>::max(),
                                                   std::numeric_limits<int32_t>::max());
        VerifySimpleAtoiGood<flare::uint128>(std::numeric_limits<uint32_t>::max(),
                                                   std::numeric_limits<uint32_t>::max());
        VerifySimpleAtoiBad<flare::uint128>(std::numeric_limits<int64_t>::min());
        VerifySimpleAtoiGood<flare::uint128>(std::numeric_limits<int64_t>::max(),
                                                   std::numeric_limits<int64_t>::max());
        VerifySimpleAtoiGood<flare::uint128>(std::numeric_limits<uint64_t>::max(),
                                                   std::numeric_limits<uint64_t>::max());
        VerifySimpleAtoiGood<flare::uint128>(
                std::numeric_limits<flare::uint128>::max(),
                std::numeric_limits<flare::uint128>::max());

        // Some other types
        VerifySimpleAtoiGood<int>(-42, -42);
        VerifySimpleAtoiGood<int32_t>(-42, -42);
        VerifySimpleAtoiGood<uint32_t>(42, 42);
        VerifySimpleAtoiGood<unsigned int>(42, 42);
        VerifySimpleAtoiGood<int64_t>(-42, -42);
        VerifySimpleAtoiGood<long>(-42, -42);  // NOLINT(runtime/int)
        VerifySimpleAtoiGood<uint64_t>(42, 42);
        VerifySimpleAtoiGood<size_t>(42, 42);
        VerifySimpleAtoiGood<std::string::size_type>(42, 42);
    }

    TEST(NumbersTest, Atoenum) {
        enum E01 {
            E01_zero = 0,
            E01_one = 1,
        };

        VerifySimpleAtoiGood<E01>(E01_zero, E01_zero);
        VerifySimpleAtoiGood<E01>(E01_one, E01_one);

        enum E_101 {
            E_101_minusone = -1,
            E_101_zero = 0,
            E_101_one = 1,
        };

        VerifySimpleAtoiGood<E_101>(E_101_minusone, E_101_minusone);
        VerifySimpleAtoiGood<E_101>(E_101_zero, E_101_zero);
        VerifySimpleAtoiGood<E_101>(E_101_one, E_101_one);

        enum E_bigint {
            E_bigint_zero = 0,
            E_bigint_one = 1,
            E_bigint_max31 = static_cast<int32_t>(0x7FFFFFFF),
        };

        VerifySimpleAtoiGood<E_bigint>(E_bigint_zero, E_bigint_zero);
        VerifySimpleAtoiGood<E_bigint>(E_bigint_one, E_bigint_one);
        VerifySimpleAtoiGood<E_bigint>(E_bigint_max31, E_bigint_max31);

        enum E_fullint {
            E_fullint_zero = 0,
            E_fullint_one = 1,
            E_fullint_max31 = static_cast<int32_t>(0x7FFFFFFF),
            E_fullint_min32 = INT32_MIN,
        };

        VerifySimpleAtoiGood<E_fullint>(E_fullint_zero, E_fullint_zero);
        VerifySimpleAtoiGood<E_fullint>(E_fullint_one, E_fullint_one);
        VerifySimpleAtoiGood<E_fullint>(E_fullint_max31, E_fullint_max31);
        VerifySimpleAtoiGood<E_fullint>(E_fullint_min32, E_fullint_min32);

        enum E_biguint {
            E_biguint_zero = 0,
            E_biguint_one = 1,
            E_biguint_max31 = static_cast<uint32_t>(0x7FFFFFFF),
            E_biguint_max32 = static_cast<uint32_t>(0xFFFFFFFF),
        };

        VerifySimpleAtoiGood<E_biguint>(E_biguint_zero, E_biguint_zero);
        VerifySimpleAtoiGood<E_biguint>(E_biguint_one, E_biguint_one);
        VerifySimpleAtoiGood<E_biguint>(E_biguint_max31, E_biguint_max31);
        VerifySimpleAtoiGood<E_biguint>(E_biguint_max32, E_biguint_max32);
    }

    TEST(stringtest, safe_strto32_base) {
        int32_t value;
        EXPECT_TRUE(safe_strto32_base("0x34234324", &value, 16));
        EXPECT_EQ(0x34234324, value);

        EXPECT_TRUE(safe_strto32_base("0X34234324", &value, 16));
        EXPECT_EQ(0x34234324, value);

        EXPECT_TRUE(safe_strto32_base("34234324", &value, 16));
        EXPECT_EQ(0x34234324, value);

        EXPECT_TRUE(safe_strto32_base("0", &value, 16));
        EXPECT_EQ(0, value);

        EXPECT_TRUE(safe_strto32_base(" \t\n -0x34234324", &value, 16));
        EXPECT_EQ(-0x34234324, value);

        EXPECT_TRUE(safe_strto32_base(" \t\n -34234324", &value, 16));
        EXPECT_EQ(-0x34234324, value);

        EXPECT_TRUE(safe_strto32_base("7654321", &value, 8));
        EXPECT_EQ(07654321, value);

        EXPECT_TRUE(safe_strto32_base("-01234", &value, 8));
        EXPECT_EQ(-01234, value);

        EXPECT_FALSE(safe_strto32_base("1834", &value, 8));

        // Autodetect base.
        EXPECT_TRUE(safe_strto32_base("0", &value, 0));
        EXPECT_EQ(0, value);

        EXPECT_TRUE(safe_strto32_base("077", &value, 0));
        EXPECT_EQ(077, value);  // Octal interpretation

        // Leading zero indicates octal, but then followed by invalid digit.
        EXPECT_FALSE(safe_strto32_base("088", &value, 0));

        // Leading 0x indicated hex, but then followed by invalid digit.
        EXPECT_FALSE(safe_strto32_base("0xG", &value, 0));

        // Base-10 version.
        EXPECT_TRUE(safe_strto32_base("34234324", &value, 10));
        EXPECT_EQ(34234324, value);

        EXPECT_TRUE(safe_strto32_base("0", &value, 10));
        EXPECT_EQ(0, value);

        EXPECT_TRUE(safe_strto32_base(" \t\n -34234324", &value, 10));
        EXPECT_EQ(-34234324, value);

        EXPECT_TRUE(safe_strto32_base("34234324 \n\t ", &value, 10));
        EXPECT_EQ(34234324, value);

        // Invalid ints.
        EXPECT_FALSE(safe_strto32_base("", &value, 10));
        EXPECT_FALSE(safe_strto32_base("  ", &value, 10));
        EXPECT_FALSE(safe_strto32_base("abc", &value, 10));
        EXPECT_FALSE(safe_strto32_base("34234324a", &value, 10));
        EXPECT_FALSE(safe_strto32_base("34234.3", &value, 10));

        // Out of bounds.
        EXPECT_FALSE(safe_strto32_base("2147483648", &value, 10));
        EXPECT_FALSE(safe_strto32_base("-2147483649", &value, 10));

        // String version.
        EXPECT_TRUE(safe_strto32_base(std::string("0x1234"), &value, 16));
        EXPECT_EQ(0x1234, value);

        // Base-10 std::string version.
        EXPECT_TRUE(safe_strto32_base("1234", &value, 10));
        EXPECT_EQ(1234, value);
    }

    TEST(stringtest, safe_strto32_range) {
        // These tests verify underflow/overflow behaviour.
        int32_t value;
        EXPECT_FALSE(safe_strto32_base("2147483648", &value, 10));
        EXPECT_EQ(std::numeric_limits<int32_t>::max(), value);

        EXPECT_TRUE(safe_strto32_base("-2147483648", &value, 10));
        EXPECT_EQ(std::numeric_limits<int32_t>::min(), value);

        EXPECT_FALSE(safe_strto32_base("-2147483649", &value, 10));
        EXPECT_EQ(std::numeric_limits<int32_t>::min(), value);
    }

    TEST(stringtest, safe_strto64_range) {
        // These tests verify underflow/overflow behaviour.
        int64_t value;
        EXPECT_FALSE(safe_strto64_base("9223372036854775808", &value, 10));
        EXPECT_EQ(std::numeric_limits<int64_t>::max(), value);

        EXPECT_TRUE(safe_strto64_base("-9223372036854775808", &value, 10));
        EXPECT_EQ(std::numeric_limits<int64_t>::min(), value);

        EXPECT_FALSE(safe_strto64_base("-9223372036854775809", &value, 10));
        EXPECT_EQ(std::numeric_limits<int64_t>::min(), value);
    }

    TEST(stringtest, safe_strto32_leading_substring) {
        // These tests verify this comment in numbers.h:
        // On error, returns false, and sets *value to: [...]
        //   conversion of leading substring if available ("123@@@" -> 123)
        //   0 if no leading substring available
        int32_t value;
        EXPECT_FALSE(safe_strto32_base("04069@@@", &value, 10));
        EXPECT_EQ(4069, value);

        EXPECT_FALSE(safe_strto32_base("04069@@@", &value, 8));
        EXPECT_EQ(0406, value);

        EXPECT_FALSE(safe_strto32_base("04069balloons", &value, 10));
        EXPECT_EQ(4069, value);

        EXPECT_FALSE(safe_strto32_base("04069balloons", &value, 16));
        EXPECT_EQ(0x4069ba, value);

        EXPECT_FALSE(safe_strto32_base("@@@", &value, 10));
        EXPECT_EQ(0, value);  // there was no leading substring
    }

    TEST(stringtest, safe_strto64_leading_substring) {
        // These tests verify this comment in numbers.h:
        // On error, returns false, and sets *value to: [...]
        //   conversion of leading substring if available ("123@@@" -> 123)
        //   0 if no leading substring available
        int64_t value;
        EXPECT_FALSE(safe_strto64_base("04069@@@", &value, 10));
        EXPECT_EQ(4069, value);

        EXPECT_FALSE(safe_strto64_base("04069@@@", &value, 8));
        EXPECT_EQ(0406, value);

        EXPECT_FALSE(safe_strto64_base("04069balloons", &value, 10));
        EXPECT_EQ(4069, value);

        EXPECT_FALSE(safe_strto64_base("04069balloons", &value, 16));
        EXPECT_EQ(0x4069ba, value);

        EXPECT_FALSE(safe_strto64_base("@@@", &value, 10));
        EXPECT_EQ(0, value);  // there was no leading substring
    }

    TEST(stringtest, safe_strto64_base) {
        int64_t value;
        EXPECT_TRUE(safe_strto64_base("0x3423432448783446", &value, 16));
        EXPECT_EQ(int64_t{0x3423432448783446}, value);

        EXPECT_TRUE(safe_strto64_base("3423432448783446", &value, 16));
        EXPECT_EQ(int64_t{0x3423432448783446}, value);

        EXPECT_TRUE(safe_strto64_base("0", &value, 16));
        EXPECT_EQ(0, value);

        EXPECT_TRUE(safe_strto64_base(" \t\n -0x3423432448783446", &value, 16));
        EXPECT_EQ(int64_t{-0x3423432448783446}, value);

        EXPECT_TRUE(safe_strto64_base(" \t\n -3423432448783446", &value, 16));
        EXPECT_EQ(int64_t{-0x3423432448783446}, value);

        EXPECT_TRUE(safe_strto64_base("123456701234567012", &value, 8));
        EXPECT_EQ(int64_t{0123456701234567012}, value);

        EXPECT_TRUE(safe_strto64_base("-017777777777777", &value, 8));
        EXPECT_EQ(int64_t{-017777777777777}, value);

        EXPECT_FALSE(safe_strto64_base("19777777777777", &value, 8));

        // Autodetect base.
        EXPECT_TRUE(safe_strto64_base("0", &value, 0));
        EXPECT_EQ(0, value);

        EXPECT_TRUE(safe_strto64_base("077", &value, 0));
        EXPECT_EQ(077, value);  // Octal interpretation

        // Leading zero indicates octal, but then followed by invalid digit.
        EXPECT_FALSE(safe_strto64_base("088", &value, 0));

        // Leading 0x indicated hex, but then followed by invalid digit.
        EXPECT_FALSE(safe_strto64_base("0xG", &value, 0));

        // Base-10 version.
        EXPECT_TRUE(safe_strto64_base("34234324487834466", &value, 10));
        EXPECT_EQ(int64_t{34234324487834466}, value);

        EXPECT_TRUE(safe_strto64_base("0", &value, 10));
        EXPECT_EQ(0, value);

        EXPECT_TRUE(safe_strto64_base(" \t\n -34234324487834466", &value, 10));
        EXPECT_EQ(int64_t{-34234324487834466}, value);

        EXPECT_TRUE(safe_strto64_base("34234324487834466 \n\t ", &value, 10));
        EXPECT_EQ(int64_t{34234324487834466}, value);

        // Invalid ints.
        EXPECT_FALSE(safe_strto64_base("", &value, 10));
        EXPECT_FALSE(safe_strto64_base("  ", &value, 10));
        EXPECT_FALSE(safe_strto64_base("abc", &value, 10));
        EXPECT_FALSE(safe_strto64_base("34234324487834466a", &value, 10));
        EXPECT_FALSE(safe_strto64_base("34234487834466.3", &value, 10));

        // Out of bounds.
        EXPECT_FALSE(safe_strto64_base("9223372036854775808", &value, 10));
        EXPECT_FALSE(safe_strto64_base("-9223372036854775809", &value, 10));

        // String version.
        EXPECT_TRUE(safe_strto64_base(std::string("0x1234"), &value, 16));
        EXPECT_EQ(0x1234, value);

        // Base-10 std::string version.
        EXPECT_TRUE(safe_strto64_base("1234", &value, 10));
        EXPECT_EQ(1234, value);
    }

    const size_t kNumRandomTests = 10000;

    template<typename IntType>
    void test_random_integer_parse_base(bool (*parse_func)(std::string_view,
                                                           IntType *value,
                                                           int base)) {
        using RandomEngine = std::minstd_rand0;
        std::random_device rd;
        RandomEngine rng(rd());
        std::uniform_int_distribution<IntType> random_int(
                std::numeric_limits<IntType>::min());
        std::uniform_int_distribution<int> random_base(2, 35);
        for (size_t i = 0; i < kNumRandomTests; i++) {
            IntType value = random_int(rng);
            int base = random_base(rng);
            std::string str_value;
            EXPECT_TRUE(Itoa<IntType>(value, base, &str_value));
            IntType parsed_value;

            // Test successful parse
            EXPECT_TRUE(parse_func(str_value, &parsed_value, base));
            EXPECT_EQ(parsed_value, value);

            // Test overflow
            EXPECT_FALSE(
                    parse_func(flare::string_cat(std::numeric_limits<IntType>::max(), value),
                               &parsed_value, base));

            // Test underflow
            if (std::numeric_limits<IntType>::min() < 0) {
                EXPECT_FALSE(
                        parse_func(flare::string_cat(std::numeric_limits<IntType>::min(), value),
                                   &parsed_value, base));
            } else {
                EXPECT_FALSE(parse_func(flare::string_cat("-", value), &parsed_value, base));
            }
        }
    }

    TEST(stringtest, safe_strto32_random) {
        test_random_integer_parse_base<int32_t>(&safe_strto32_base);
    }

    TEST(stringtest, safe_strto64_random) {
        test_random_integer_parse_base<int64_t>(&safe_strto64_base);
    }

    TEST(stringtest, safe_strtou32_random) {
        test_random_integer_parse_base<uint32_t>(&safe_strtou32_base);
    }

    TEST(stringtest, safe_strtou64_random) {
        test_random_integer_parse_base<uint64_t>(&safe_strtou64_base);
    }

    TEST(stringtest, safe_strtou128_random) {
        // random number generators don't work for uint128, and
        // uint128 can be streamed but not string_cat'd, so this code must be custom
        // implemented for uint128, but is generally the same as what's above.
        // test_random_integer_parse_base<flare::uint128>(
        //     &flare::numbers_internal::safe_strtou128_base);
        using RandomEngine = std::minstd_rand0;
        using IntType = flare::uint128;
        constexpr auto parse_func = &flare::numbers_internal::safe_strtou128_base;

        std::random_device rd;
        RandomEngine rng(rd());
        std::uniform_int_distribution<uint64_t> random_uint64(
                std::numeric_limits<uint64_t>::min());
        std::uniform_int_distribution<int> random_base(2, 35);

        for (size_t i = 0; i < kNumRandomTests; i++) {
            IntType value = random_uint64(rng);
            value = (value << 64) + random_uint64(rng);
            int base = random_base(rng);
            std::string str_value;
            EXPECT_TRUE(Itoa<IntType>(value, base, &str_value));
            IntType parsed_value;

            // Test successful parse
            EXPECT_TRUE(parse_func(str_value, &parsed_value, base));
            EXPECT_EQ(parsed_value, value);

            // Test overflow
            std::string s;
            flare::strings_internal::string_output_stream(&s)
                    << std::numeric_limits<IntType>::max() << value;
            EXPECT_FALSE(parse_func(s, &parsed_value, base));

            // Test underflow
            s.clear();
            flare::strings_internal::string_output_stream(&s) << "-" << value;
            EXPECT_FALSE(parse_func(s, &parsed_value, base));
        }
    }

    TEST(stringtest, safe_strtou32_base) {
        for (int i = 0; strtouint32_test_cases()[i].str != nullptr; ++i) {
            const auto &e = strtouint32_test_cases()[i];
            uint32_t value;
            EXPECT_EQ(e.expect_ok, safe_strtou32_base(e.str, &value, e.base))
                                << "str=\"" << e.str << "\" base=" << e.base;
            if (e.expect_ok) {
                EXPECT_EQ(e.expected, value) << "i=" << i << " str=\"" << e.str
                                             << "\" base=" << e.base;
            }
        }
    }

    TEST(stringtest, safe_strtou32_base_length_delimited) {
        for (int i = 0; strtouint32_test_cases()[i].str != nullptr; ++i) {
            const auto &e = strtouint32_test_cases()[i];
            std::string tmp(e.str);
            tmp.append("12");  // Adds garbage at the end.

            uint32_t value;
            EXPECT_EQ(e.expect_ok,
                      safe_strtou32_base(std::string_view(tmp.data(), strlen(e.str)),
                                         &value, e.base))
                                << "str=\"" << e.str << "\" base=" << e.base;
            if (e.expect_ok) {
                EXPECT_EQ(e.expected, value) << "i=" << i << " str=" << e.str
                                             << " base=" << e.base;
            }
        }
    }

    TEST(stringtest, safe_strtou64_base) {
        for (int i = 0; strtouint64_test_cases()[i].str != nullptr; ++i) {
            const auto &e = strtouint64_test_cases()[i];
            uint64_t value;
            EXPECT_EQ(e.expect_ok, safe_strtou64_base(e.str, &value, e.base))
                                << "str=\"" << e.str << "\" base=" << e.base;
            if (e.expect_ok) {
                EXPECT_EQ(e.expected, value) << "str=" << e.str << " base=" << e.base;
            }
        }
    }

    TEST(stringtest, safe_strtou64_base_length_delimited) {
        for (int i = 0; strtouint64_test_cases()[i].str != nullptr; ++i) {
            const auto &e = strtouint64_test_cases()[i];
            std::string tmp(e.str);
            tmp.append("12");  // Adds garbage at the end.

            uint64_t value;
            EXPECT_EQ(e.expect_ok,
                      safe_strtou64_base(std::string_view(tmp.data(), strlen(e.str)),
                                         &value, e.base))
                                << "str=\"" << e.str << "\" base=" << e.base;
            if (e.expect_ok) {
                EXPECT_EQ(e.expected, value) << "str=\"" << e.str << "\" base=" << e.base;
            }
        }
    }

// feenableexcept() and fedisableexcept() are extensions supported by some libc
// implementations.
#if defined(__GLIBC__) || defined(__BIONIC__)
#define FLARE_HAVE_FEENABLEEXCEPT 1
#define FLARE_HAVE_FEDISABLEEXCEPT 1
#endif

    class SimpleDtoaTest : public testing::Test {
    protected:
        void SetUp() override {
            // Store the current floating point env & clear away any pending exceptions.
            feholdexcept(&fp_env_);
#ifdef FLARE_HAVE_FEENABLEEXCEPT
            // Turn on floating point exceptions.
            feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif
        }

        void TearDown() override {
            // Restore the floating point environment to the original state.
            // In theory fedisableexcept is unnecessary; fesetenv will also do it.
            // In practice, our toolchains have subtle bugs.
#ifdef FLARE_HAVE_FEDISABLEEXCEPT
            fedisableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif
            fesetenv(&fp_env_);
        }

        std::string ToNineDigits(double value) {
            char buffer[16];  // more than enough for %.9g
            snprintf(buffer, sizeof(buffer), "%.9g", value);
            return buffer;
        }

        fenv_t fp_env_;
    };

// Run the given runnable functor for "cases" test cases, chosen over the
// available range of float.  pi and e and 1/e are seeded, and then all
// available integer powers of 2 and 10 are multiplied against them.  In
// addition to trying all those values, we try the next higher and next lower
// float, and then we add additional test cases evenly distributed between them.
// Each test case is passed to runnable as both a positive and negative value.
    template<typename R>
    void ExhaustiveFloat(uint32_t cases, R &&runnable) {
        runnable(0.0f);
        runnable(-0.0f);
        if (cases >= 2e9) {  // more than 2 billion?  Might as well run them all.
            for (float f = 0; f < std::numeric_limits<float>::max();) {
                f = nextafterf(f, std::numeric_limits<float>::max());
                runnable(-f);
                runnable(f);
            }
            return;
        }
        std::set<float> floats = {3.4028234e38f};
        for (float f : {1.0, 3.14159265, 2.718281828, 1 / 2.718281828}) {
            for (float testf = f; testf != 0; testf *= 0.1f) floats.insert(testf);
            for (float testf = f; testf != 0; testf *= 0.5f) floats.insert(testf);
            for (float testf = f; testf < 3e38f / 2; testf *= 2.0f)
                floats.insert(testf);
            for (float testf = f; testf < 3e38f / 10; testf *= 10) floats.insert(testf);
        }

        float last = *floats.begin();

        runnable(last);
        runnable(-last);
        int iters_per_float = cases / floats.size();
        if (iters_per_float == 0) iters_per_float = 1;
        for (float f : floats) {
            if (f == last) continue;
            float testf = std::nextafter(last, std::numeric_limits<float>::max());
            runnable(testf);
            runnable(-testf);
            last = testf;
            if (f == last) continue;
            double step = (double{f} - last) / iters_per_float;
            for (double d = last + step; d < f; d += step) {
                testf = d;
                if (testf != last) {
                    runnable(testf);
                    runnable(-testf);
                    last = testf;
                }
            }
            testf = std::nextafter(f, 0.0f);
            if (testf > last) {
                runnable(testf);
                runnable(-testf);
                last = testf;
            }
            if (f != last) {
                runnable(f);
                runnable(-f);
                last = f;
            }
        }
    }

    TEST_F(SimpleDtoaTest, ExhaustiveDoubleToSixDigits) {
        uint64_t test_count = 0;
        std::vector<double> mismatches;
        auto checker = [&](double d) {
            if (d != d) return;  // rule out NaNs
            ++test_count;
            char sixdigitsbuf[kSixDigitsToBufferSize] = {0};
            six_digits_to_buffer(d, sixdigitsbuf);
            char snprintfbuf[kSixDigitsToBufferSize] = {0};
            snprintf(snprintfbuf, kSixDigitsToBufferSize, "%g", d);
            if (strcmp(sixdigitsbuf, snprintfbuf) != 0) {
                mismatches.push_back(d);
                if (mismatches.size() < 10) {
                    FLARE_LOG(ERROR) <<
                               flare::string_cat("Six-digit failure with double.  ", "d=", d,
                                                          "=", d, " sixdigits=", sixdigitsbuf,
                                                          " printf(%g)=", snprintfbuf).c_str();
                }
            }
        };
        // Some quick sanity checks...
        checker(5e-324);
        checker(1e-308);
        checker(1.0);
        checker(1.000005);
        checker(1.7976931348623157e308);
        checker(0.00390625);
#ifndef _MSC_VER
        // on MSVC, snprintf() rounds it to 0.00195313. six_digits_to_buffer() rounds it
        // to 0.00195312 (round half to even).
        checker(0.001953125);
#endif
        checker(0.005859375);
        // Some cases where the rounding is very very close
        checker(1.089095e-15);
        checker(3.274195e-55);
        checker(6.534355e-146);
        checker(2.920845e+234);

        if (mismatches.empty()) {
            test_count = 0;
            ExhaustiveFloat(kFloatNumCases, checker);

            test_count = 0;
            std::vector<int> digit_testcases{
                    100000, 100001, 100002, 100005, 100010, 100020, 100050, 100100,  // misc
                    195312, 195313,  // 1.953125 is a case where we round down, just barely.
                    200000, 500000, 800000,  // misc mid-range cases
                    585937, 585938,  // 5.859375 is a case where we round up, just barely.
                    900000, 990000, 999000, 999900, 999990, 999996, 999997, 999998, 999999};
            if (kFloatNumCases >= 1e9) {
                // If at least 1 billion test cases were requested, user wants an
                // exhaustive test. So let's test all mantissas, too.
                constexpr int min_mantissa = 100000, max_mantissa = 999999;
                digit_testcases.resize(max_mantissa - min_mantissa + 1);
                std::iota(digit_testcases.begin(), digit_testcases.end(), min_mantissa);
            }

            for (int exponent = -324; exponent <= 308; ++exponent) {
                double powten = Pow10(exponent);
                if (powten == 0) powten = 5e-324;
                if (kFloatNumCases >= 1e9) {
                    // The exhaustive test takes a very long time, so log progress.
                    char buf[kSixDigitsToBufferSize];
                    FLARE_LOG(INFO) <<
                              flare::string_cat("Exp ", exponent, " powten=", powten, "(", powten,
                                                         ") (",
                                                         std::string(buf, six_digits_to_buffer(powten, buf)), ")")
                                      .c_str();
                }
                for (int digits : digit_testcases) {
                    if (exponent == 308 && digits >= 179769) break;  // don't overflow!
                    double digiform = (digits + 0.5) * 0.00001;
                    double testval = digiform * powten;
                    double pretestval = nextafter(testval, 0);
                    double posttestval = nextafter(testval, 1.7976931348623157e308);
                    checker(testval);
                    checker(pretestval);
                    checker(posttestval);
                }
            }
        } else {
            EXPECT_EQ(mismatches.size(), 0UL);
            for (size_t i = 0; i < mismatches.size(); ++i) {
                if (i > 100) i = mismatches.size() - 1;
                double d = mismatches[i];
                char sixdigitsbuf[kSixDigitsToBufferSize] = {0};
                six_digits_to_buffer(d, sixdigitsbuf);
                char snprintfbuf[kSixDigitsToBufferSize] = {0};
                snprintf(snprintfbuf, kSixDigitsToBufferSize, "%g", d);
                double before = nextafter(d, 0.0);
                double after = nextafter(d, 1.7976931348623157e308);
                char b1[32], b2[kSixDigitsToBufferSize];
                FLARE_LOG(ERROR) <<
                           flare::string_cat(
                                   "Mismatch #", i, "  d=", d, " (", ToNineDigits(d), ")",
                                   " sixdigits='", sixdigitsbuf, "'", " snprintf='", snprintfbuf,
                                   "'", " Before.=", PerfectDtoa(before), " ",
                                   (six_digits_to_buffer(before, b2), b2),
                                   " vs snprintf=", (snprintf(b1, sizeof(b1), "%g", before), b1),
                                   " Perfect=", PerfectDtoa(d), " ", (six_digits_to_buffer(d, b2), b2),
                                   " vs snprintf=", (snprintf(b1, sizeof(b1), "%g", d), b1),
                                   " After.=.", PerfectDtoa(after), " ",
                                   (six_digits_to_buffer(after, b2), b2),
                                   " vs snprintf=", (snprintf(b1, sizeof(b1), "%g", after), b1))
                                   .c_str();
            }
        }
    }

    TEST(StrToInt32, Partial) {
        struct Int32TestLine {
            std::string input;
            bool status;
            int32_t value;
        };
        const int32_t int32_min = std::numeric_limits<int32_t>::min();
        const int32_t int32_max = std::numeric_limits<int32_t>::max();
        Int32TestLine int32_test_line[] = {
                {"",                                               false, 0},
                {" ",                                              false, 0},
                {"-",                                              false, 0},
                {"123@@@",                                         false, 123},
                {flare::string_cat(int32_min, int32_max), false, int32_min},
                {flare::string_cat(int32_max, int32_max), false, int32_max},
        };

        for (const Int32TestLine &test_line : int32_test_line) {
            int32_t value = -2;
            bool status = safe_strto32_base(test_line.input, &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
            value = -2;
            status = safe_strto32_base(test_line.input, &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
            value = -2;
            status = safe_strto32_base(std::string_view(test_line.input), &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
        }
    }

    TEST(StrToUint32, Partial) {
        struct Uint32TestLine {
            std::string input;
            bool status;
            uint32_t value;
        };
        const uint32_t uint32_max = std::numeric_limits<uint32_t>::max();
        Uint32TestLine uint32_test_line[] = {
                {"",                                                 false, 0},
                {" ",                                                false, 0},
                {"-",                                                false, 0},
                {"123@@@",                                           false, 123},
                {flare::string_cat(uint32_max, uint32_max), false, uint32_max},
        };

        for (const Uint32TestLine &test_line : uint32_test_line) {
            uint32_t value = 2;
            bool status = safe_strtou32_base(test_line.input, &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
            value = 2;
            status = safe_strtou32_base(test_line.input, &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
            value = 2;
            status = safe_strtou32_base(std::string_view(test_line.input), &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
        }
    }

    TEST(Strto_int64, Partial) {
        struct Int64TestLine {
            std::string input;
            bool status;
            int64_t value;
        };
        const int64_t int64_min = std::numeric_limits<int64_t>::min();
        const int64_t int64_max = std::numeric_limits<int64_t>::max();
        Int64TestLine int64_test_line[] = {
                {"",                                               false, 0},
                {" ",                                              false, 0},
                {"-",                                              false, 0},
                {"123@@@",                                         false, 123},
                {flare::string_cat(int64_min, int64_max), false, int64_min},
                {flare::string_cat(int64_max, int64_max), false, int64_max},
        };

        for (const Int64TestLine &test_line : int64_test_line) {
            int64_t value = -2;
            bool status = safe_strto64_base(test_line.input, &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
            value = -2;
            status = safe_strto64_base(test_line.input, &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
            value = -2;
            status = safe_strto64_base(std::string_view(test_line.input), &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
        }
    }

    TEST(StrToUint64, Partial) {
        struct Uint64TestLine {
            std::string input;
            bool status;
            uint64_t value;
        };
        const uint64_t uint64_max = std::numeric_limits<uint64_t>::max();
        Uint64TestLine uint64_test_line[] = {
                {"",                                                 false, 0},
                {" ",                                                false, 0},
                {"-",                                                false, 0},
                {"123@@@",                                           false, 123},
                {flare::string_cat(uint64_max, uint64_max), false, uint64_max},
        };

        for (const Uint64TestLine &test_line : uint64_test_line) {
            uint64_t value = 2;
            bool status = safe_strtou64_base(test_line.input, &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
            value = 2;
            status = safe_strtou64_base(test_line.input, &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
            value = 2;
            status = safe_strtou64_base(std::string_view(test_line.input), &value, 10);
            EXPECT_EQ(test_line.status, status) << test_line.input;
            EXPECT_EQ(test_line.value, value) << test_line.input;
        }
    }

    TEST(StrToInt32Base, PrefixOnly) {
        struct Int32TestLine {
            std::string input;
            bool status;
            int32_t value;
        };
        Int32TestLine int32_test_line[] = {
                {"",    false, 0},
                {"-",   false, 0},
                {"-0",  true,  0},
                {"0",   true,  0},
                {"0x",  false, 0},
                {"-0x", false, 0},
        };
        const int base_array[] = {0, 2, 8, 10, 16};

        for (const Int32TestLine &line : int32_test_line) {
            for (const int base : base_array) {
                int32_t value = 2;
                bool status = safe_strto32_base(line.input.c_str(), &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
                value = 2;
                status = safe_strto32_base(line.input, &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
                value = 2;
                status = safe_strto32_base(std::string_view(line.input), &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
            }
        }
    }

    TEST(StrToUint32Base, PrefixOnly) {
        struct Uint32TestLine {
            std::string input;
            bool status;
            uint32_t value;
        };
        Uint32TestLine uint32_test_line[] = {
                {"",   false, 0},
                {"0",  true,  0},
                {"0x", false, 0},
        };
        const int base_array[] = {0, 2, 8, 10, 16};

        for (const Uint32TestLine &line : uint32_test_line) {
            for (const int base : base_array) {
                uint32_t value = 2;
                bool status = safe_strtou32_base(line.input.c_str(), &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
                value = 2;
                status = safe_strtou32_base(line.input, &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
                value = 2;
                status = safe_strtou32_base(std::string_view(line.input), &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
            }
        }
    }

    TEST(StrToInt64Base, PrefixOnly) {
        struct Int64TestLine {
            std::string input;
            bool status;
            int64_t value;
        };
        Int64TestLine int64_test_line[] = {
                {"",    false, 0},
                {"-",   false, 0},
                {"-0",  true,  0},
                {"0",   true,  0},
                {"0x",  false, 0},
                {"-0x", false, 0},
        };
        const int base_array[] = {0, 2, 8, 10, 16};

        for (const Int64TestLine &line : int64_test_line) {
            for (const int base : base_array) {
                int64_t value = 2;
                bool status = safe_strto64_base(line.input.c_str(), &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
                value = 2;
                status = safe_strto64_base(line.input, &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
                value = 2;
                status = safe_strto64_base(std::string_view(line.input), &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
            }
        }
    }

    TEST(StrToUint64Base, PrefixOnly) {
        struct Uint64TestLine {
            std::string input;
            bool status;
            uint64_t value;
        };
        Uint64TestLine uint64_test_line[] = {
                {"",   false, 0},
                {"0",  true,  0},
                {"0x", false, 0},
        };
        const int base_array[] = {0, 2, 8, 10, 16};

        for (const Uint64TestLine &line : uint64_test_line) {
            for (const int base : base_array) {
                uint64_t value = 2;
                bool status = safe_strtou64_base(line.input.c_str(), &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
                value = 2;
                status = safe_strtou64_base(line.input, &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
                value = 2;
                status = safe_strtou64_base(std::string_view(line.input), &value, base);
                EXPECT_EQ(line.status, status) << line.input << " " << base;
                EXPECT_EQ(line.value, value) << line.input << " " << base;
            }
        }
    }

    void TestFastHexToBufferZeroPad16(uint64_t v) {
        char buf[16];
        auto digits = flare::numbers_internal::fast_hex_to_buffer_zero_pad16(v, buf);
        std::string_view res(buf, 16);
        char buf2[17];
        snprintf(buf2, sizeof(buf2), "%016" PRIx64, v);
        EXPECT_EQ(res, buf2) << v;
        size_t expected_digits = snprintf(buf2, sizeof(buf2), "%" PRIx64, v);
        EXPECT_EQ(digits, expected_digits) << v;
    }

    TEST(fast_hex_to_buffer_zero_pad16, Smoke) {
        TestFastHexToBufferZeroPad16(std::numeric_limits<uint64_t>::min());
        TestFastHexToBufferZeroPad16(std::numeric_limits<uint64_t>::max());
        TestFastHexToBufferZeroPad16(std::numeric_limits<int64_t>::min());
        TestFastHexToBufferZeroPad16(std::numeric_limits<int64_t>::max());
        for (int i = 0; i < 100000; ++i) {
            TestFastHexToBufferZeroPad16(
                    flare::base::fast_rand_in(std::numeric_limits<uint64_t>::min(),
                                              std::numeric_limits<uint64_t>::max()));
        }
    }

}  // namespace
