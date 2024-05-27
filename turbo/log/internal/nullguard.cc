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

#include <turbo/log/internal/nullguard.h>

#include <array>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

TURBO_CONST_INIT TURBO_DLL const std::array<char, 7> kCharNull{
    {'(', 'n', 'u', 'l', 'l', ')', '\0'}};
TURBO_CONST_INIT TURBO_DLL const std::array<signed char, 7> kSignedCharNull{
    {'(', 'n', 'u', 'l', 'l', ')', '\0'}};
TURBO_CONST_INIT TURBO_DLL const std::array<unsigned char, 7> kUnsignedCharNull{
    {'(', 'n', 'u', 'l', 'l', ')', '\0'}};

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
