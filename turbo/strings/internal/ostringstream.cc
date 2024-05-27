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

#include <turbo/strings/internal/ostringstream.h>

#include <cassert>
#include <cstddef>
#include <ios>
#include <streambuf>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {

OStringStream::Streambuf::int_type OStringStream::Streambuf::overflow(int c) {
  assert(str_);
  if (!std::streambuf::traits_type::eq_int_type(
          c, std::streambuf::traits_type::eof()))
    str_->push_back(static_cast<char>(c));
  return 1;
}

std::streamsize OStringStream::Streambuf::xsputn(const char* s,
                                                 std::streamsize n) {
  assert(str_);
  str_->append(s, static_cast<size_t>(n));
  return n;
}

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo
