// Copyright 2013-2023 Daniel Parker
// Copyright 2023 The Turbo Authors.
//
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

#ifndef TURBO_JSONCONS_ALLOCATOR_HOLDER_H_
#define TURBO_JSONCONS_ALLOCATOR_HOLDER_H_

namespace turbo {

    template<class Allocator>
    class allocator_holder {
    public:
        using allocator_type = Allocator;
    private:
        allocator_type alloc_;
    public:
        allocator_holder() = default;

        allocator_holder(const allocator_holder &) = default;

        allocator_holder(allocator_holder &&) = default;

        allocator_holder &operator=(const allocator_holder &) = default;

        allocator_holder &operator=(allocator_holder &&) = default;

        allocator_holder(const allocator_type &alloc)
                : alloc_(alloc) {}

        ~allocator_holder() = default;

        allocator_type get_allocator() const {
            return alloc_;
        }
    };

}  // namespace turbo

#endif  // TURBO_JSONCONS_ALLOCATOR_HOLDER_H_

