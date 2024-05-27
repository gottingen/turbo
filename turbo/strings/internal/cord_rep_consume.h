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

#ifndef TURBO_STRINGS_INTERNAL_CORD_REP_CONSUME_H_
#define TURBO_STRINGS_INTERNAL_CORD_REP_CONSUME_H_

#include <functional>

#include <turbo/functional/function_ref.h>
#include <turbo/strings/internal/cord_internal.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace cord_internal {

// Consume() and ReverseConsume() consume CONCAT based trees and invoke the
// provided functor with the contained nodes in the proper forward or reverse
// order, which is used to convert CONCAT trees into other tree or cord data.
// All CONCAT and SUBSTRING nodes are processed internally. The 'offset`
// parameter of the functor is non-zero for any nodes below SUBSTRING nodes.
// It's up to the caller to form these back into SUBSTRING nodes or otherwise
// store offset / prefix information. These functions are intended to be used
// only for migration / transitional code where due to factors such as ODR
// violations, we can not 100% guarantee that all code respects 'new format'
// settings and flags, so we need to be able to parse old data on the fly until
// all old code is deprecated / no longer the default format.
void Consume(CordRep* rep,
             FunctionRef<void(CordRep*, size_t, size_t)> consume_fn);
void ReverseConsume(CordRep* rep,
                    FunctionRef<void(CordRep*, size_t, size_t)> consume_fn);

}  // namespace cord_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_CORD_REP_CONSUME_H_
