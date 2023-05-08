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

#ifndef JSONCONS_JSONMERGEPATCH_JSONMERGEPATCH_HPP
#define JSONCONS_JSONMERGEPATCH_JSONMERGEPATCH_HPP

#include <string>
#include <vector> 
#include <memory>
#include <algorithm> // std::min
#include <utility> // std::move
#include "turbo/jsoncons/json.h"

namespace turbo {
namespace mergepatch {

    template <class Json>
    Json from_diff(const Json& source, const Json& target)
    {
        if (!source.is_object() || !target.is_object())
        {
            return target;
        }
        Json result(json_object_arg);

        for (const auto& member : source.object_range())
        {
            auto it = target.find(member.key());
            if (it != target.object_range().end())
            {
                if (member.value() != it->value())
                {
                    result.try_emplace(member.key(), from_diff(member.value(), it->value()));
                }
            }
            else
            {
                result.try_emplace(member.key(), Json::null());
            }
        }

        for (const auto& member : target.object_range())
        {
            auto it = source.find(member.key());
            if (it == source.object_range().end())
            {
                result.try_emplace(member.key(), member.value());
            }
        }

        return result;
    }

    namespace detail {
        template <class Json>
        Json apply_merge_patch_(Json& target, const Json& patch)
        {
            if (patch.is_object())
            {
                if (!target.is_object())
                {
                    target = Json(json_object_arg);
                }
                for (auto& member : patch.object_range())
                {
                    auto it = target.find(member.key());
                    if (it != target.object_range().end())
                    {
                        Json item = it->value();
                        target.erase(it);
                        if (!member.value().is_null())
                        {
                            target.try_emplace(member.key(), apply_merge_patch_(item, member.value()));
                        }
                    }
                    else if (!member.value().is_null())
                    {
                        Json item(json_object_arg);
                        target.try_emplace(member.key(), apply_merge_patch_(item, member.value()));
                    }
                }
                return target;
            }
            else
            {
                return patch;
            }
        }
    } // namespace detail

    template <class Json>
    void apply_merge_patch(Json& target, const Json& patch)
    {
        target = detail::apply_merge_patch_(target, patch);
    }

} // namespace mergepatch
} // namespace turbo

#endif
