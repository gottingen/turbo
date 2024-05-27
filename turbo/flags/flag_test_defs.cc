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

// This file is used to test the mismatch of the flag type between definition
// and declaration. These are definitions. flag_test.cc contains declarations.
#include <string>
#include <turbo/flags/flag.h>

TURBO_FLAG(int, mistyped_int_flag, 0, "");
TURBO_FLAG(std::string, mistyped_string_flag, "", "");
TURBO_FLAG(bool, flag_on_separate_file, false, "");
TURBO_RETIRED_FLAG(bool, retired_flag_on_separate_file, false, "");
