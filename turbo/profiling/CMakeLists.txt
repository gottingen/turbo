# Copyright 2021 The Turbo Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


turbo_cc_test(
  NAME
    sample_recorder_test
  SRCS
    "internal/sample_recorder_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    turbo::sync
    GTest::gmock_main
)

turbo_cc_test(
  NAME
    exponential_biased_test
  SRCS
    "internal/exponential_biased_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)


turbo_cc_test(
  NAME
    periodic_sampler_test
  SRCS
    "internal/periodic_sampler_test.cc"
  COPTS
    ${TURBO_TEST_COPTS}
  DEPS
    turbo::turbo
    GTest::gmock_main
)

