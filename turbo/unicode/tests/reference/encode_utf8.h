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

namespace turbo { namespace tests { namespace reference { namespace utf8 {

  template <typename CONSUMER>
  int encode(uint32_t value, CONSUMER consumer) {
      if (value < 0x00000080) {
          consumer(uint8_t(value));
          return 1;
      }

      if (value < 0x00000800) {
          consumer(0xc0 | (value >> 6));
          consumer(0x80 | (value & 0x3f));
          return 2;
      }

      if (value < 0x00010000) {
          consumer(0xe0 | (value >> 12));
          consumer(0x80 | ((value >> 6) & 0x3f));
          consumer(0x80 | (value & 0x3f));
          return 3;
      }

      {
          consumer(0xf0 | (value >> 18));
          consumer(0x80 | ((value >> 12) & 0x3f));
          consumer(0x80 | ((value >> 6) & 0x3f));
          consumer(0x80 | (value & 0x3f));
          return 4;
      }
  }
}}}} // namespace utf8
