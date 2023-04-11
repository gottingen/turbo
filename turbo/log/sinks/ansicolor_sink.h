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

#ifndef TURBO_LOG_SINKS_ANSICOLOR_SINK_H_
#define TURBO_LOG_SINKS_ANSICOLOR_SINK_H_


#include <array>
#include <memory>
#include <mutex>
#include <cstdio>
#include <string>
#include "turbo/log/log_sink.h"
#include "turbo/strings/string_view.h"

namespace turbo {

/**
 * This sink prefixes the output with an ANSI escape sequence color code
 * depending on the severity
 * of the message.
 * If no color terminal detected, omit the escape codes.
 */

class AnsicolorSink : public LogSink {
public:
  AnsicolorSink(FILE *target_file);
  ~AnsicolorSink() override = default;

  void Send(const turbo::LogEntry& entry) override;

  void Flush() override;
  // Formatting codes
  const std::string_view reset = "\033[m";
  const std::string_view bold = "\033[1m";
  const std::string_view dark = "\033[2m";
  const std::string_view underline = "\033[4m";
  const std::string_view blink = "\033[5m";
  const std::string_view reverse = "\033[7m";
  const std::string_view concealed = "\033[8m";
  const std::string_view clear_line = "\033[K";

  // Foreground colors
  const std::string_view black = "\033[30m";
  const std::string_view red = "\033[31m";
  const std::string_view green = "\033[32m";
  const std::string_view yellow = "\033[33m";
  const std::string_view blue = "\033[34m";
  const std::string_view magenta = "\033[35m";
  const std::string_view cyan = "\033[36m";
  const std::string_view white = "\033[37m";

  /// Background colors
  const std::string_view on_black = "\033[40m";
  const std::string_view on_red = "\033[41m";
  const std::string_view on_green = "\033[42m";
  const std::string_view on_yellow = "\033[43m";
  const std::string_view on_blue = "\033[44m";
  const std::string_view on_magenta = "\033[45m";
  const std::string_view on_cyan = "\033[46m";
  const std::string_view on_white = "\033[47m";

  /// Bold colors
  const std::string_view yellow_bold = "\033[33m\033[1m";
  const std::string_view red_bold = "\033[31m\033[1m";
  const std::string_view bold_on_red = "\033[1m\033[41m";

private:
  TURBO_NON_COPYABLE(AnsicolorSink);
  TURBO_NON_MOVEABLE(AnsicolorSink);
  FILE *target_file_;
  std::array<std::string, 5> colors_;
};

} // namespace turbo

#endif // TURBO_LOG_SINKS_ANSICOLOR_SINK_H_