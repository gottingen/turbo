// Copyright 2022 The Turbo Authors.
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
#include "turbo/strings/string_piece.h"

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
  const string_piece reset = "\033[m";
  const string_piece bold = "\033[1m";
  const string_piece dark = "\033[2m";
  const string_piece underline = "\033[4m";
  const string_piece blink = "\033[5m";
  const string_piece reverse = "\033[7m";
  const string_piece concealed = "\033[8m";
  const string_piece clear_line = "\033[K";

  // Foreground colors
  const string_piece black = "\033[30m";
  const string_piece red = "\033[31m";
  const string_piece green = "\033[32m";
  const string_piece yellow = "\033[33m";
  const string_piece blue = "\033[34m";
  const string_piece magenta = "\033[35m";
  const string_piece cyan = "\033[36m";
  const string_piece white = "\033[37m";

  /// Background colors
  const string_piece on_black = "\033[40m";
  const string_piece on_red = "\033[41m";
  const string_piece on_green = "\033[42m";
  const string_piece on_yellow = "\033[43m";
  const string_piece on_blue = "\033[44m";
  const string_piece on_magenta = "\033[45m";
  const string_piece on_cyan = "\033[46m";
  const string_piece on_white = "\033[47m";

  /// Bold colors
  const string_piece yellow_bold = "\033[33m\033[1m";
  const string_piece red_bold = "\033[31m\033[1m";
  const string_piece bold_on_red = "\033[1m\033[41m";

private:
  TURBO_NON_COPYABLE(AnsicolorSink);
  TURBO_NON_MOVEABLE(AnsicolorSink);
  FILE *target_file_;
  std::array<std::string, 5> colors_;
};

} // namespace turbo

#endif // TURBO_LOG_SINKS_ANSICOLOR_SINK_H_