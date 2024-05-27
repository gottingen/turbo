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

#include <gtest/gtest.h>
#include <turbo/base/optimization.h>
#include <turbo/strings/string_view.h>

// This test by itself does not do anything fancy, but it serves as binary I can
// query in shell test.

namespace {

template <class T>
void DoNotOptimize(const T& var) {
#ifdef __GNUC__
  asm volatile("" : "+m"(const_cast<T&>(var)));
#else
  std::cout << (void*)&var;
#endif
}

int very_long_int_variable_name TURBO_INTERNAL_UNIQUE_SMALL_NAME() = 0;
char very_long_str_variable_name[] TURBO_INTERNAL_UNIQUE_SMALL_NAME() = "abc";

TEST(UniqueSmallName, NonAutomaticVar) {
  EXPECT_EQ(very_long_int_variable_name, 0);
  EXPECT_EQ(turbo::string_view(very_long_str_variable_name), "abc");
}

int VeryLongFreeFunctionName() TURBO_INTERNAL_UNIQUE_SMALL_NAME();

TEST(UniqueSmallName, FreeFunction) {
  DoNotOptimize(&VeryLongFreeFunctionName);

  EXPECT_EQ(VeryLongFreeFunctionName(), 456);
}

int VeryLongFreeFunctionName() { return 456; }

struct VeryLongStructName {
  explicit VeryLongStructName(int i);

  int VeryLongMethodName() TURBO_INTERNAL_UNIQUE_SMALL_NAME();

  static int VeryLongStaticMethodName() TURBO_INTERNAL_UNIQUE_SMALL_NAME();

 private:
  int fld;
};

TEST(UniqueSmallName, Struct) {
  VeryLongStructName var(10);

  DoNotOptimize(var);
  DoNotOptimize(&VeryLongStructName::VeryLongMethodName);
  DoNotOptimize(&VeryLongStructName::VeryLongStaticMethodName);

  EXPECT_EQ(var.VeryLongMethodName(), 10);
  EXPECT_EQ(VeryLongStructName::VeryLongStaticMethodName(), 123);
}

VeryLongStructName::VeryLongStructName(int i) : fld(i) {}
int VeryLongStructName::VeryLongMethodName() { return fld; }
int VeryLongStructName::VeryLongStaticMethodName() { return 123; }

}  // namespace
