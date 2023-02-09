// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/strings/substitute.h"

#include <cstdint>
#include <vector>

#include "gtest/gtest.h"
#include "turbo/strings/str_cat.h"

namespace {

struct MyStruct {
  template <typename Sink>
  friend void TurboStringify(Sink& sink, const MyStruct& s) {
    sink.Append("MyStruct{.value = ");
    sink.Append(turbo::StrCat(s.value));
    sink.Append("}");
  }
  int value;
};

TEST(SubstituteTest, Substitute) {
  // Basic.
  EXPECT_EQ("Hello, world!", turbo::Substitute("$0, $1!", "Hello", "world"));

  // Non-char* types.
  EXPECT_EQ("123 0.2 0.1 foo true false x",
            turbo::Substitute("$0 $1 $2 $3 $4 $5 $6", 123, 0.2, 0.1f,
                             std::string("foo"), true, false, 'x'));

  // All int types.
  EXPECT_EQ(
      "-32767 65535 "
      "-1234567890 3234567890 "
      "-1234567890 3234567890 "
      "-1234567890123456789 9234567890123456789",
      turbo::Substitute(
          "$0 $1 $2 $3 $4 $5 $6 $7",
          static_cast<short>(-32767),          // NOLINT(runtime/int)
          static_cast<unsigned short>(65535),  // NOLINT(runtime/int)
          -1234567890, 3234567890U, -1234567890L, 3234567890UL,
          -int64_t{1234567890123456789}, uint64_t{9234567890123456789u}));

  // Hex format
  EXPECT_EQ("0 1 f ffff0ffff 0123456789abcdef",
            turbo::Substitute("$0$1$2$3$4 $5",  //
                             turbo::Hex(0), turbo::Hex(1, turbo::kSpacePad2),
                             turbo::Hex(0xf, turbo::kSpacePad2),
                             turbo::Hex(int16_t{-1}, turbo::kSpacePad5),
                             turbo::Hex(int16_t{-1}, turbo::kZeroPad5),
                             turbo::Hex(0x123456789abcdef, turbo::kZeroPad16)));

  // Dec format
  EXPECT_EQ("0 115   -1-0001 81985529216486895",
            turbo::Substitute("$0$1$2$3$4 $5",  //
                             turbo::Dec(0), turbo::Dec(1, turbo::kSpacePad2),
                             turbo::Dec(0xf, turbo::kSpacePad2),
                             turbo::Dec(int16_t{-1}, turbo::kSpacePad5),
                             turbo::Dec(int16_t{-1}, turbo::kZeroPad5),
                             turbo::Dec(0x123456789abcdef, turbo::kZeroPad16)));

  // Pointer.
  const int* int_p = reinterpret_cast<const int*>(0x12345);
  std::string str = turbo::Substitute("$0", int_p);
  EXPECT_EQ(turbo::StrCat("0x", turbo::Hex(int_p)), str);

  // Volatile Pointer.
  // Like C++ streamed I/O, such pointers implicitly become bool
  volatile int vol = 237;
  volatile int* volatile volptr = &vol;
  str = turbo::Substitute("$0", volptr);
  EXPECT_EQ("true", str);

  // null is special. StrCat prints 0x0. Substitute prints NULL.
  const uint64_t* null_p = nullptr;
  str = turbo::Substitute("$0", null_p);
  EXPECT_EQ("NULL", str);

  // char* is also special.
  const char* char_p = "print me";
  str = turbo::Substitute("$0", char_p);
  EXPECT_EQ("print me", str);

  char char_buf[16];
  strncpy(char_buf, "print me too", sizeof(char_buf));
  str = turbo::Substitute("$0", char_buf);
  EXPECT_EQ("print me too", str);

  // null char* is "doubly" special. Represented as the empty string.
  char_p = nullptr;
  str = turbo::Substitute("$0", char_p);
  EXPECT_EQ("", str);

  // Out-of-order.
  EXPECT_EQ("b, a, c, b", turbo::Substitute("$1, $0, $2, $1", "a", "b", "c"));

  // Literal $
  EXPECT_EQ("$", turbo::Substitute("$$"));

  EXPECT_EQ("$1", turbo::Substitute("$$1"));

  // Test all overloads.
  EXPECT_EQ("a", turbo::Substitute("$0", "a"));
  EXPECT_EQ("a b", turbo::Substitute("$0 $1", "a", "b"));
  EXPECT_EQ("a b c", turbo::Substitute("$0 $1 $2", "a", "b", "c"));
  EXPECT_EQ("a b c d", turbo::Substitute("$0 $1 $2 $3", "a", "b", "c", "d"));
  EXPECT_EQ("a b c d e",
            turbo::Substitute("$0 $1 $2 $3 $4", "a", "b", "c", "d", "e"));
  EXPECT_EQ("a b c d e f", turbo::Substitute("$0 $1 $2 $3 $4 $5", "a", "b", "c",
                                            "d", "e", "f"));
  EXPECT_EQ("a b c d e f g", turbo::Substitute("$0 $1 $2 $3 $4 $5 $6", "a", "b",
                                              "c", "d", "e", "f", "g"));
  EXPECT_EQ("a b c d e f g h",
            turbo::Substitute("$0 $1 $2 $3 $4 $5 $6 $7", "a", "b", "c", "d", "e",
                             "f", "g", "h"));
  EXPECT_EQ("a b c d e f g h i",
            turbo::Substitute("$0 $1 $2 $3 $4 $5 $6 $7 $8", "a", "b", "c", "d",
                             "e", "f", "g", "h", "i"));
  EXPECT_EQ("a b c d e f g h i j",
            turbo::Substitute("$0 $1 $2 $3 $4 $5 $6 $7 $8 $9", "a", "b", "c",
                             "d", "e", "f", "g", "h", "i", "j"));
  EXPECT_EQ("a b c d e f g h i j b0",
            turbo::Substitute("$0 $1 $2 $3 $4 $5 $6 $7 $8 $9 $10", "a", "b", "c",
                             "d", "e", "f", "g", "h", "i", "j"));

  const char* null_cstring = nullptr;
  EXPECT_EQ("Text: ''", turbo::Substitute("Text: '$0'", null_cstring));

  MyStruct s1 = MyStruct{17};
  MyStruct s2 = MyStruct{1043};
  EXPECT_EQ("MyStruct{.value = 17}, MyStruct{.value = 1043}",
            turbo::Substitute("$0, $1", s1, s2));
}

TEST(SubstituteTest, SubstituteAndAppend) {
  std::string str = "Hello";
  turbo::SubstituteAndAppend(&str, ", $0!", "world");
  EXPECT_EQ("Hello, world!", str);

  // Test all overloads.
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0", "a");
  EXPECT_EQ("a", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1", "a", "b");
  EXPECT_EQ("a b", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1 $2", "a", "b", "c");
  EXPECT_EQ("a b c", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1 $2 $3", "a", "b", "c", "d");
  EXPECT_EQ("a b c d", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4", "a", "b", "c", "d", "e");
  EXPECT_EQ("a b c d e", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5", "a", "b", "c", "d", "e",
                            "f");
  EXPECT_EQ("a b c d e f", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5 $6", "a", "b", "c", "d",
                            "e", "f", "g");
  EXPECT_EQ("a b c d e f g", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5 $6 $7", "a", "b", "c", "d",
                            "e", "f", "g", "h");
  EXPECT_EQ("a b c d e f g h", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5 $6 $7 $8", "a", "b", "c",
                            "d", "e", "f", "g", "h", "i");
  EXPECT_EQ("a b c d e f g h i", str);
  str.clear();
  turbo::SubstituteAndAppend(&str, "$0 $1 $2 $3 $4 $5 $6 $7 $8 $9", "a", "b",
                            "c", "d", "e", "f", "g", "h", "i", "j");
  EXPECT_EQ("a b c d e f g h i j", str);

  str.clear();
  MyStruct s1 = MyStruct{17};
  MyStruct s2 = MyStruct{1043};
  turbo::SubstituteAndAppend(&str, "$0, $1", s1, s2);
  EXPECT_EQ("MyStruct{.value = 17}, MyStruct{.value = 1043}", str);
}

TEST(SubstituteTest, VectorBoolRef) {
  std::vector<bool> v = {true, false};
  const auto& cv = v;
  EXPECT_EQ("true false true false",
            turbo::Substitute("$0 $1 $2 $3", v[0], v[1], cv[0], cv[1]));

  std::string str = "Logic be like: ";
  turbo::SubstituteAndAppend(&str, "$0 $1 $2 $3", v[0], v[1], cv[0], cv[1]);
  EXPECT_EQ("Logic be like: true false true false", str);
}

TEST(SubstituteTest, Enums) {
  enum UnscopedEnum { kEnum0 = 0, kEnum1 = 1 };
  EXPECT_EQ("0 1", turbo::Substitute("$0 $1", UnscopedEnum::kEnum0,
                                    UnscopedEnum::kEnum1));

  enum class ScopedEnum { kEnum0 = 0, kEnum1 = 1 };
  EXPECT_EQ("0 1",
            turbo::Substitute("$0 $1", ScopedEnum::kEnum0, ScopedEnum::kEnum1));

  enum class ScopedEnumInt32 : int32_t { kEnum0 = 989, kEnum1 = INT32_MIN };
  EXPECT_EQ("989 -2147483648",
            turbo::Substitute("$0 $1", ScopedEnumInt32::kEnum0,
                             ScopedEnumInt32::kEnum1));

  enum class ScopedEnumUInt32 : uint32_t { kEnum0 = 1, kEnum1 = UINT32_MAX };
  EXPECT_EQ("1 4294967295", turbo::Substitute("$0 $1", ScopedEnumUInt32::kEnum0,
                                             ScopedEnumUInt32::kEnum1));

  enum class ScopedEnumInt64 : int64_t { kEnum0 = -1, kEnum1 = 42949672950 };
  EXPECT_EQ("-1 42949672950", turbo::Substitute("$0 $1", ScopedEnumInt64::kEnum0,
                                               ScopedEnumInt64::kEnum1));

  enum class ScopedEnumUInt64 : uint64_t { kEnum0 = 1, kEnum1 = 42949672950 };
  EXPECT_EQ("1 42949672950", turbo::Substitute("$0 $1", ScopedEnumUInt64::kEnum0,
                                              ScopedEnumUInt64::kEnum1));

  enum class ScopedEnumChar : signed char { kEnum0 = -1, kEnum1 = 1 };
  EXPECT_EQ("-1 1", turbo::Substitute("$0 $1", ScopedEnumChar::kEnum0,
                                     ScopedEnumChar::kEnum1));

  enum class ScopedEnumUChar : unsigned char {
    kEnum0 = 0,
    kEnum1 = 1,
    kEnumMax = 255
  };
  EXPECT_EQ("0 1 255", turbo::Substitute("$0 $1 $2", ScopedEnumUChar::kEnum0,
                                        ScopedEnumUChar::kEnum1,
                                        ScopedEnumUChar::kEnumMax));

  enum class ScopedEnumInt16 : int16_t { kEnum0 = -100, kEnum1 = 10000 };
  EXPECT_EQ("-100 10000", turbo::Substitute("$0 $1", ScopedEnumInt16::kEnum0,
                                           ScopedEnumInt16::kEnum1));

  enum class ScopedEnumUInt16 : uint16_t { kEnum0 = 0, kEnum1 = 10000 };
  EXPECT_EQ("0 10000", turbo::Substitute("$0 $1", ScopedEnumUInt16::kEnum0,
                                        ScopedEnumUInt16::kEnum1));
}

enum class EnumWithStringify { Many = 0, Choices = 1 };

template <typename Sink>
void TurboStringify(Sink& sink, EnumWithStringify e) {
  sink.Append(e == EnumWithStringify::Many ? "Many" : "Choices");
}

TEST(SubstituteTest, TurboStringifyWithEnum) {
  const auto e = EnumWithStringify::Choices;
  EXPECT_EQ(turbo::Substitute("$0", e), "Choices");
}

#if GTEST_HAS_DEATH_TEST

TEST(SubstituteDeathTest, SubstituteDeath) {
  EXPECT_DEBUG_DEATH(
      static_cast<void>(turbo::Substitute(turbo::string_view("-$2"), "a", "b")),
      "Invalid turbo::Substitute\\(\\) format string: asked for \"\\$2\", "
      "but only 2 args were given.");
  EXPECT_DEBUG_DEATH(
      static_cast<void>(turbo::Substitute(turbo::string_view("-$z-"))),
      "Invalid turbo::Substitute\\(\\) format string: \"-\\$z-\"");
  EXPECT_DEBUG_DEATH(
      static_cast<void>(turbo::Substitute(turbo::string_view("-$"))),
      "Invalid turbo::Substitute\\(\\) format string: \"-\\$\"");
}

#endif  // GTEST_HAS_DEATH_TEST

}  // namespace
