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

#include <turbo/strings/internal/cordz_sample_token.h>

#include <turbo/base/config.h>
#include <turbo/strings/internal/cordz_handle.h>
#include <turbo/strings/internal/cordz_info.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

CordzSampleToken::Iterator& CordzSampleToken::Iterator::operator++() {
  if (current_) {
    current_ = current_->Next(*token_);
  }
  return *this;
}

CordzSampleToken::Iterator CordzSampleToken::Iterator::operator++(int) {
  Iterator it(*this);
  operator++();
  return it;
}

bool operator==(const CordzSampleToken::Iterator& lhs,
                const CordzSampleToken::Iterator& rhs) {
  return lhs.current_ == rhs.current_ &&
         (lhs.current_ == nullptr || lhs.token_ == rhs.token_);
}

bool operator!=(const CordzSampleToken::Iterator& lhs,
                const CordzSampleToken::Iterator& rhs) {
  return !(lhs == rhs);
}

CordzSampleToken::Iterator::reference CordzSampleToken::Iterator::operator*()
    const {
  return *current_;
}

CordzSampleToken::Iterator::pointer CordzSampleToken::Iterator::operator->()
    const {
  return current_;
}

CordzSampleToken::Iterator::Iterator(const CordzSampleToken* token)
    : token_(token), current_(CordzInfo::Head(*token)) {}

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo
