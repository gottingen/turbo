// Copyright 2023 The Turbo Authors.
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

#include "turbo/var/reducer.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include "turbo/flags/reflection.h"

class FileDumperTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(FileDumperTest, filters) {
    turbo::Adder<int> a1("a_latency");
    turbo::Adder<int> a2("a_qps");
    turbo::Adder<int> a3("a_error");
    turbo::Adder<int> a4("process_*");
    turbo::Adder<int> a5("default");
    turbo::set_command_line_flag("var_dump_interval", "1");
    turbo::set_command_line_flag("var_dump", "true");
    sleep(2);
}
