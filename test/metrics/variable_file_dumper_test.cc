// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// Date: 2015/08/27 17:12:38

#include "testing/gtest_wrap.h"
#include "flare/metrics/gauge.h"
#include <gflags/gflags.h>
#include <cstdlib>

class FileDumperTest : public testing::Test {
protected:
    void SetUp() {}
    void TearDown() {}
};

TEST_F(FileDumperTest, filters) {
    flare::gauge<int> a1("a_latency");
    flare::gauge<int> a2("a_qps");
    flare::gauge<int> a3("a_error");
    flare::gauge<int> a4("process_*");
    flare::gauge<int> a5("default");
    google::SetCommandLineOption("variable_dump_interval", "1");
    google::SetCommandLineOption("variable_dump", "true");
    sleep(2);
}
