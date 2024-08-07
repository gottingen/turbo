//
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
//
// Created by jeff on 24-6-29.
//

#pragma once

#if defined(__GNUC__)  // GCC or clang
#define TURBO_DIAGNOSTIC_PUSH _Pragma("GCC diagnostic push")
#define TURBO_DIAGNOSTIC_POP _Pragma("GCC diagnostic pop")

#define TURBO_DIAGNOSTIC_IGNORE_DEPRECATED _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")

#elif defined(_MSC_VER)
#define TURBO_DIAGNOSTIC_PUSH __pragma(warning(push))
#define TURBO_DIAGNOSTIC_POP __pragma(warning(pop))

#define TURBO_DIAGNOSTIC_IGNORE_DEPRECATED __pragma(warning(disable : 4996))

#else
#define TURBO_DIAGNOSTIC_PUSH
#define TURBO_DIAGNOSTIC_POP

#define TURBO_DIAGNOSTIC_IGNORE_DEPRECATED

#endif