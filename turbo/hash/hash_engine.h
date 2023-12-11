// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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

#ifndef TURBO_HASH_HASH_ENGINE_H_
#define TURBO_HASH_HASH_ENGINE_H_

#include "turbo/hash/fwd.h"
#include "turbo/hash/mix/murmur_mix.h"
#include "turbo/hash/mix/simple_mix.h"
#include "turbo/hash/city/city.h"
#include "turbo/hash/bytes/bytes_hash.h"

namespace turbo {

#ifdef TURBO_DEFAUT_HASH_ENGINE
using default_hash_engine = TURBO_DEFAUT_HASH_ENGINE;
#else
using default_hash_engine = bytes_hash_tag;
#endif
}  // namespace turbo

#endif  // TURBO_HASH_HASH_ENGINE_H_
