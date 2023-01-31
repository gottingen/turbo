//
// Copyright 2022 The Turbo Authors.
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
//

#include "turbo/strings/internal/str_format/extension.h"

#include <random>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/strings/str_format.h"
#include "turbo/strings/string_view.h"

namespace my_namespace {
class UserDefinedType {
 public:
  UserDefinedType() = default;

  void Append(std::string_view str) { value_.append(str.data(), str.size()); }
  const std::string& Value() const { return value_; }

  friend void TurboFormatFlush(UserDefinedType* x, std::string_view str) {
    x->Append(str);
  }

 private:
  std::string value_;
};
}  // namespace my_namespace

namespace {

std::string MakeRandomString(size_t len) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis('a', 'z');
  std::string s(len, '0');
  for (char& c : s) {
    c = dis(gen);
  }
  return s;
}

TEST(FormatExtensionTest, SinkAppendSubstring) {
  for (size_t chunk_size : {1, 10, 100, 1000, 10000}) {
    std::string expected, actual;
    turbo::str_format_internal::FormatSinkImpl sink(&actual);
    for (size_t chunks = 0; chunks < 10; ++chunks) {
      std::string rand = MakeRandomString(chunk_size);
      expected += rand;
      sink.Append(rand);
    }
    sink.Flush();
    EXPECT_EQ(actual, expected);
  }
}

TEST(FormatExtensionTest, SinkAppendChars) {
  for (size_t chunk_size : {1, 10, 100, 1000, 10000}) {
    std::string expected, actual;
    turbo::str_format_internal::FormatSinkImpl sink(&actual);
    for (size_t chunks = 0; chunks < 10; ++chunks) {
      std::string rand = MakeRandomString(1);
      expected.append(chunk_size, rand[0]);
      sink.Append(chunk_size, rand[0]);
    }
    sink.Flush();
    EXPECT_EQ(actual, expected);
  }
}

TEST(FormatExtensionTest, VerifyEnumEquality) {
#define X_VAL(id)                           \
  EXPECT_EQ(turbo::FormatConversionChar::id, \
            turbo::str_format_internal::FormatConversionCharInternal::id);
  TURBO_INTERNAL_CONVERSION_CHARS_EXPAND_(X_VAL, );
#undef X_VAL

#define X_VAL(id)                              \
  EXPECT_EQ(turbo::FormatConversionCharSet::id, \
            turbo::str_format_internal::FormatConversionCharSetInternal::id);
  TURBO_INTERNAL_CONVERSION_CHARS_EXPAND_(X_VAL, );
#undef X_VAL
}

TEST(FormatExtensionTest, SetConversionChar) {
  turbo::str_format_internal::FormatConversionSpecImpl spec;
  EXPECT_EQ(spec.conversion_char(),
            turbo::str_format_internal::FormatConversionCharInternal::kNone);
  spec.set_conversion_char(
      turbo::str_format_internal::FormatConversionCharInternal::d);
  EXPECT_EQ(spec.conversion_char(),
            turbo::str_format_internal::FormatConversionCharInternal::d);
}

}  // namespace
