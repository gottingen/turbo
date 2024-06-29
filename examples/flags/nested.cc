// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include <turbo/flags/app.h>
#include <string>

int main(int argc, char **argv) {

    turbo::cli::App app("Vision Application");
    app.set_help_all_flag("--help-all", "Expand all help");
    app.add_flag("--version", "Get version");

    turbo::cli::App *cameraApp = app.add_subcommand("camera", "Configure the app camera");
    cameraApp->require_subcommand(0, 1);  // 0 (default) or 1 camera

    std::string mvcamera_config_file = "mvcamera_config.json";
    turbo::cli::App *mvcameraApp = cameraApp->add_subcommand("mvcamera", "MatrixVision Camera Configuration");
    mvcameraApp->add_option("-c,--config", mvcamera_config_file, "Config filename")
        ->capture_default_str()
        ->check(turbo::cli::ExistingFile);

    std::string mock_camera_path;
    turbo::cli::App *mockcameraApp = cameraApp->add_subcommand("mock", "Mock Camera Configuration");
    mockcameraApp->add_option("-p,--path", mock_camera_path, "Path")->required()->check(turbo::cli::ExistingPath);

    TURBO_APP_PARSE(app, argc, argv);
}
