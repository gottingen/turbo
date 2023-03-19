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

#include "turbo/log/sinks/ansicolor_sink.h"
#include "turbo/log/globals.h"

namespace turbo {
AnsicolorSink::AnsicolorSink(FILE *target_file) : target_file_(target_file){
  colors_[static_cast<size_t>(LogSeverity::kInfo)] = std::string(green.data(),green.size());
  colors_[static_cast<size_t>(LogSeverity::kWarning)] = std::string(blue.data(), blue.size());
  colors_[static_cast<size_t>(LogSeverity::kError)] = std::string(yellow.data(), yellow.size());
  colors_[static_cast<size_t>(LogSeverity::kFatal)] = std::string(red_bold.data(),red_bold.size());
  colors_[4] = std::string(reset.data(),reset.size());
}

void AnsicolorSink::Send(const turbo::LogEntry& entry) {
  string_piece color = colors_[static_cast<size_t>(entry.log_severity())];
  string_piece reset = colors_[4];
  string_piece text =entry.text_message_with_prefix_and_newline();
  fwrite(color.data(),color.size(), 1,target_file_);
  fwrite(text.data(), text.size(), 1, target_file_);
  fwrite(reset.data(), reset.size(), 1, target_file_);
}

void AnsicolorSink::Flush() {
  fflush(target_file_);
}

}  // namespace turbo
