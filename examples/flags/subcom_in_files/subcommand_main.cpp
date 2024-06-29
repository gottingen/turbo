// Copyright (c) 2017-2024, University of Cincinnati, developed by Henry Schreiner
// under NSF AWARD 1414736 and by the respective contributors.
// All rights reserved.
//
// SPDX-License-Identifier: BSD-3-Clause

#include "subcommand_a.hpp"
#include <turbo/flags/app.h>

int main(int argc, char **argv) {
    turbo::cli::App app{"..."};

    // Call the setup functions for the subcommands.
    // They are kept alive by a shared pointer in the
    // lambda function held by CLI11
    setup_subcommand_a(app);

    // Make sure we get at least one subcommand
    app.require_subcommand();

    // More setup if needed, i.e., other subcommands etc.

    TURBO_APP_PARSE(app, argc, argv);

    return 0;
}
