#
# Copyright 2024 The Carbin Authors.
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

set(${PROJECT_NAME}_SKIP_TEST "")
set(${PROJECT_NAME}_SKIP_BENCHMARK "")

list(APPEND ${PROJECT_NAME}_SKIP_TEST
        "norun"
        #[[  "base"
          "container"
          "algorithm"
          "cleanup"
          "crc"
          "debugging"
          "flags"
          "functional"]]
)
list(APPEND ${PROJECT_NAME}_SKIP_BENCHMARK "norun"
        "base"
        "container"
        "crc"
        "debugging"
        "flags"
        "functional"
        "hash"
        "log"
        "numeric"
        "profile"
        "random"
        "strings"
        "synchronization"
        "times"
        "types"
)