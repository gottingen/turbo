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

#include <turbo/strings/internal/resize_uninitialized.h>

#include <gtest/gtest.h>

namespace {

int resize_call_count = 0;
int append_call_count = 0;

// A mock string class whose only purpose is to track how many times its
// resize()/append() methods have been called.
struct resizable_string {
  using value_type = char;
  size_t size() const { return 0; }
  size_t capacity() const { return 0; }
  char& operator[](size_t) {
    static char c = '\0';
    return c;
  }
  void resize(size_t) { resize_call_count += 1; }
  void append(size_t, value_type) { append_call_count += 1; }
  void reserve(size_t) {}
  resizable_string& erase(size_t = 0, size_t = 0) { return *this; }
};

int resize_default_init_call_count = 0;
int append_default_init_call_count = 0;

// A mock string class whose only purpose is to track how many times its
// resize()/__resize_default_init()/append()/__append_default_init() methods
// have been called.
struct default_init_string {
  size_t size() const { return 0; }
  size_t capacity() const { return 0; }
  char& operator[](size_t) {
    static char c = '\0';
    return c;
  }
  void resize(size_t) { resize_call_count += 1; }
  void __resize_default_init(size_t) { resize_default_init_call_count += 1; }
  void __append_default_init(size_t) { append_default_init_call_count += 1; }
  void reserve(size_t) {}
  default_init_string& erase(size_t = 0, size_t = 0) { return *this; }
};

TEST(ResizeUninit, WithAndWithout) {
  resize_call_count = 0;
  append_call_count = 0;
  resize_default_init_call_count = 0;
  append_default_init_call_count = 0;
  {
    resizable_string rs;

    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(append_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    EXPECT_EQ(append_default_init_call_count, 0);
    EXPECT_FALSE(
        turbo::strings_internal::STLStringSupportsNontrashingResize(&rs));
    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(append_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    EXPECT_EQ(append_default_init_call_count, 0);
    turbo::strings_internal::STLStringResizeUninitialized(&rs, 237);
    EXPECT_EQ(resize_call_count, 1);
    EXPECT_EQ(append_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    EXPECT_EQ(append_default_init_call_count, 0);
    turbo::strings_internal::STLStringResizeUninitializedAmortized(&rs, 1000);
    EXPECT_EQ(resize_call_count, 1);
    EXPECT_EQ(append_call_count, 1);
    EXPECT_EQ(resize_default_init_call_count, 0);
    EXPECT_EQ(append_default_init_call_count, 0);
  }

  resize_call_count = 0;
  append_call_count = 0;
  resize_default_init_call_count = 0;
  append_default_init_call_count = 0;
  {
    default_init_string rus;

    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(append_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    EXPECT_EQ(append_default_init_call_count, 0);
    EXPECT_TRUE(
        turbo::strings_internal::STLStringSupportsNontrashingResize(&rus));
    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(append_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 0);
    EXPECT_EQ(append_default_init_call_count, 0);
    turbo::strings_internal::STLStringResizeUninitialized(&rus, 237);
    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(append_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 1);
    EXPECT_EQ(append_default_init_call_count, 0);
    turbo::strings_internal::STLStringResizeUninitializedAmortized(&rus, 1000);
    EXPECT_EQ(resize_call_count, 0);
    EXPECT_EQ(append_call_count, 0);
    EXPECT_EQ(resize_default_init_call_count, 1);
    EXPECT_EQ(append_default_init_call_count, 1);
  }
}

TEST(ResizeUninit, Amortized) {
  std::string str;
  size_t prev_cap = str.capacity();
  int cap_increase_count = 0;
  for (int i = 0; i < 1000; ++i) {
    turbo::strings_internal::STLStringResizeUninitializedAmortized(&str, i);
    size_t new_cap = str.capacity();
    if (new_cap > prev_cap) ++cap_increase_count;
    prev_cap = new_cap;
  }
  EXPECT_LT(cap_increase_count, 50);
}

}  // namespace
