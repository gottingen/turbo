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

#pragma once

#include <cstdint>

namespace turbo { namespace tests { namespace reference { namespace utf16 {

  // returns whether the value can be represented in the UTF-16
  bool valid_value(uint32_t value);

  // Encodes the value using either one or two words (returns 1 or 2 respectively)
  // Returns 0 if the value cannot be encoded
  int encode(uint32_t value, char16_t& W1, char16_t& W2);

}}}} // namespace utf16
