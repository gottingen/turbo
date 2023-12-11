// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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

#include "turbo/tools/hash.h"
#include "turbo/tools/context.h"
#include "turbo/format/table.h"
#include "turbo/format/format.h"
#include "turbo/hash/hash_old.h"

namespace turbo::tools {
    void set_up_hash_cmdline(turbo::App &app) {
        auto hcmd = app.add_subcommand("hash", "hash a string or a file");
        hcmd->add_option("-s, --string", Context::get_instance().hash_string, "hash a string");
        hcmd->callback([]() { run_hash_string(); });
    }


    void run_hash_string() {
        turbo::Table result;
        result.add_row({"original", turbo::tools::Context::get_instance().hash_string});
        result[0].format().font_color(turbo::Color::yellow);
        result.add_row(
                {"hash", turbo::Format(turbo::Hash<std::string>{}(turbo::tools::Context::get_instance().hash_string))});
        result[1].format().font_color(turbo::Color::green);
        std::cout << result << std::endl;
    }
}  // namespace turbo::tools