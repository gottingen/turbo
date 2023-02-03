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

#ifndef TURBO_WORKFLOW_CORE_ERROR_H_
#define TURBO_WORKFLOW_CORE_ERROR_H_

#include <iostream>
#include <sstream>
#include <exception>

namespace turbo {
TURBO_NAMESPACE_BEGIN

    // Procedure: throw_se
    // Throws the system error under a given error code.
        template<typename... ArgsT>
    //void throw_se(const char* fname, const size_t line, Error::Code c, ArgsT&&... args) {
    void throw_re(const char *fname, const size_t line, ArgsT &&... args) {
        std::ostringstream oss;
        oss << "[" << fname << ":" << line << "] ";
        (oss << ... << args);
        throw std::runtime_error(oss.str());
    }
TURBO_NAMESPACE_END
}  // namespce turbo

#define TF_THROW(...) turbo::throw_re(__FILE__, __LINE__, __VA_ARGS__);

#endif  // TURBO_WORKFLOW_CORE_ERROR_H_
