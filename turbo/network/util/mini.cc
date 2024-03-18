// Copyright 2023 The turbo Authors.
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

#include "turbo/network/util/mini.h"


using namespace std;

namespace turbo {

template<>
mINI_basic<string, variant> &mINI_basic<string, variant>::Instance(){
    static mINI_basic<string, variant> instance;
    return instance;
}

template <>
bool variant::as<bool>() const {
    if (empty() || isdigit(front())) {
        //数字开头
        return as_default<bool>();
    }
    if (strToLower(std::string(*this)) == "true") {
        return true;
    }
    if (strToLower(std::string(*this)) == "false") {
        return false;
    }
    //未识别字符串
    return as_default<bool>();
}

template<>
uint8_t variant::as<uint8_t>() const {
    return 0xFF & as_default<int>();
}

}  // namespace turbo
