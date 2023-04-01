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

#include "turbo/strings/str_cat.h"
#include <iostream>
#include <string>

void str_cat_example();
int main() {
  str_cat_example();
  return 0;
}

void str_cat_example() {
  int a = 1;
  std::string b = "hello";

  /// "1hello"
  std::cout << turbo::StrCat(a,b)<<std::endl;
}