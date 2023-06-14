#
# Copyright 2023 The Carbin Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
include(carbin_option)
include(carbin_print)
include(carbin_cc_library)
include(carbin_cc_test)
include(carbin_cc_binary)
include(carbin_cc_benchmark)
include(carbin_print_list)
include(carbin_check)
include(carbin_variable)
include(carbin_include)
include(carbin_outof_source)
include(carbin_platform)
include(carbin_pkg_dump)

CARBIN_ENSURE_OUT_OF_SOURCE_BUILD("must out of source dir")

#if (NOT DEV_MODE AND ${PROJECT_VERSION} MATCHES "0.0.0")
#    carbin_error("PROJECT_VERSION must be set in file project_profile or set -DDEV_MODE=true for develop debug")
#endif()

include(carbin_cxx_config)
include(carbin_deps)


