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

#ifndef  TURBO_VAR_FLAG_VARIABLE_H_
#define  TURBO_VAR_FLAG_VARIABLE_H_

#include <string>                       // std::string
#include "turbo/var/variable.h"

namespace turbo {

    class FlagVariable : public Variable {
    public:
        FlagVariable(const std::string &gflag_name);

        FlagVariable(const std::string &prefix,
                     const std::string &gflag_name);

        // Calling hide() in dtor manually is a MUST required by Variable.
        ~FlagVariable() { hide(); }

        void describe(std::ostream &os, bool quote_string) const override;

        std::string get_value() const;

        // Set the flag with a new value.
        // Returns true on success.
        bool set_value(const char *value);

        // name of the flag.
        const std::string &flag_name() const {
            return _flag_name.empty() ? name() : _flag_name;
        }

    private:
        std::string _flag_name;
    };

}  // namespace turbo

#endif  // TURBO_VAR_FLAG_VARIABLE_H_
