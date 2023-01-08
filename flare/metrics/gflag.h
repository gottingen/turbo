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

// Date: Sun Aug  9 12:26:03 CST 2015

#ifndef  FLARE_VARIABLE_GFLAG_H_
#define  FLARE_VARIABLE_GFLAG_H_

#include <string>                       // std::string
#include "flare/metrics/variable_base.h"

namespace flare {

    // Expose important gflags as variable so that they're monitored.
    class metrics_gflag : public variable_base {
    public:
        metrics_gflag(const std::string_view &gflag_name);

        metrics_gflag(const std::string_view &prefix,
              const std::string_view &gflag_name);

        // Calling hide() in dtor manually is a MUST required by variable_base.
        ~metrics_gflag() { hide(); }

        void describe(std::ostream &os, bool quote_string) const override;


        // Get value of the gflag.
        // We don't bother making the return type generic. This function
        // is just for consistency with other classes.
        std::string get_value() const;

        // Set the gflag with a new value.
        // Returns true on success.
        bool set_value(const char *value);

        // name of the gflag.
        const std::string &gflag_name() const {
            return _gflag_name.empty() ? name() : _gflag_name;
        }

    private:
        std::string _gflag_name;
    };

}  // namespace flare

#endif  // FLARE_VARIABLE_GFLAG_H_
