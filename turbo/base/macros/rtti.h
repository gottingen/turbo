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

#ifndef TURBO_USE_STATIC_RTTI
#if(defined(_HAS_STATIC_RTTI) && _HAS_STATIC_RTTI)
#define TURBO_USE_STATIC_RTTI 1
#elif defined(__cpp_rtti)
#if(defined(_CPPRTTI) && _CPPRTTI == 0)
#define TURBO_USE_STATIC_RTTI 1
#else
#define TURBO_USE_STATIC_RTTI 0
#endif
#elif(defined(__GCC_RTTI) && __GXX_RTTI)
#define TURBO_USE_STATIC_RTTI 0
#else
#define TURBO_USE_STATIC_RTTI 1
#endif
#endif
