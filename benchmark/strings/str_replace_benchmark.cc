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

#include <turbo/strings/str_replace.h>

#include <cstring>
#include <string>

#include <benchmark/benchmark.h>
#include <turbo/base/internal/raw_logging.h>

namespace {

std::string* big_string;
std::string* after_replacing_the;
std::string* after_replacing_many;

struct Replacement {
  const char* needle;
  const char* replacement;
} replacements[] = {
    {"the", "box"},          //
    {"brown", "quick"},      //
    {"jumped", "liquored"},  //
    {"dozen", "brown"},      //
    {"lazy", "pack"},        //
    {"liquor", "shakes"},    //
};

// Here, we set up a string for use in global-replace benchmarks.
// We started with a million blanks, and then deterministically insert
// 10,000 copies each of two pangrams.  The result is a string that is
// 40% blank space and 60% these words.  'the' occurs 18,247 times and
// all the substitutions together occur 49,004 times.
//
// We then create "after_replacing_the" to be a string that is a result of
// replacing "the" with "box" in big_string.
//
// And then we create "after_replacing_many" to be a string that is result
// of preferring several substitutions.
void SetUpStrings() {
  if (big_string == nullptr) {
    size_t r = 0;
    big_string = new std::string(1000 * 1000, ' ');
    for (std::string phrase : {"the quick brown fox jumped over the lazy dogs",
                               "pack my box with the five dozen liquor jugs"}) {
      for (int i = 0; i < 10 * 1000; ++i) {
        r = r * 237 + 41;  // not very random.
        memcpy(&(*big_string)[r % (big_string->size() - phrase.size())],
               phrase.data(), phrase.size());
      }
    }
    // big_string->resize(50);
    // OK, we've set up the string, now let's set up expectations - first by
    // just replacing "the" with "box"
    after_replacing_the = new std::string(*big_string);
    for (size_t pos = 0;
         (pos = after_replacing_the->find("the", pos)) != std::string::npos;) {
      memcpy(&(*after_replacing_the)[pos], "box", 3);
    }
    // And then with all the replacements.
    after_replacing_many = new std::string(*big_string);
    for (size_t pos = 0;;) {
      size_t next_pos = static_cast<size_t>(-1);
      const char* needle_string = nullptr;
      const char* replacement_string = nullptr;
      for (const auto& r : replacements) {
        auto needlepos = after_replacing_many->find(r.needle, pos);
        if (needlepos != std::string::npos && needlepos < next_pos) {
          next_pos = needlepos;
          needle_string = r.needle;
          replacement_string = r.replacement;
        }
      }
      if (next_pos > after_replacing_many->size()) break;
      after_replacing_many->replace(next_pos, strlen(needle_string),
                                    replacement_string);
      next_pos += strlen(replacement_string);
      pos = next_pos;
    }
  }
}

void BM_StrReplaceAllOneReplacement(benchmark::State& state) {
  SetUpStrings();
  std::string src = *big_string;
  for (auto _ : state) {
    std::string dest = turbo::StrReplaceAll(src, {{"the", "box"}});
    TURBO_RAW_CHECK(dest == *after_replacing_the,
                   "not benchmarking intended behavior");
  }
}
BENCHMARK(BM_StrReplaceAllOneReplacement);

void BM_StrReplaceAll(benchmark::State& state) {
  SetUpStrings();
  std::string src = *big_string;
  for (auto _ : state) {
    std::string dest = turbo::StrReplaceAll(src, {{"the", "box"},
                                                 {"brown", "quick"},
                                                 {"jumped", "liquored"},
                                                 {"dozen", "brown"},
                                                 {"lazy", "pack"},
                                                 {"liquor", "shakes"}});
    TURBO_RAW_CHECK(dest == *after_replacing_many,
                   "not benchmarking intended behavior");
  }
}
BENCHMARK(BM_StrReplaceAll);

}  // namespace
