//
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

#include "turbo/log/internal/test_actions.h"

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>

#include "turbo/platform/port.h"
#include "turbo/strings/escaping.h"
#include "turbo/strings/str_cat.h"
#include "turbo/strings/string_piece.h"
#include "turbo/time/time.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

void WriteToStderrWithFilename::operator()(const turbo::LogEntry& entry) const {
  std::cerr << message << " (file: " << entry.source_filename() << ")\n";
}

void WriteEntryToStderr::operator()(const turbo::LogEntry& entry) const {
  if (!message.empty()) std::cerr << message << "\n";

  const std::string source_filename = turbo::CHexEscape(entry.source_filename());
  const std::string source_basename = turbo::CHexEscape(entry.source_basename());
  const std::string text_message = turbo::CHexEscape(entry.text_message());
  const std::string encoded_message = turbo::CHexEscape(entry.encoded_message());
  std::string encoded_message_str;
  std::cerr << "LogEntry{\n"                                               //
            << "  source_filename: \"" << source_filename << "\"\n"        //
            << "  source_basename: \"" << source_basename << "\"\n"        //
            << "  source_line: " << entry.source_line() << "\n"            //
            << "  prefix: " << (entry.prefix() ? "true\n" : "false\n")     //
            << "  log_severity: " << entry.log_severity() << "\n"          //
            << "  timestamp: " << entry.timestamp() << "\n"                //
            << "  text_message: \"" << text_message << "\"\n"              //
            << "  verbosity: " << entry.verbosity() << "\n"                //
            << "  encoded_message (raw): \"" << encoded_message << "\"\n"  //
            << encoded_message_str                                         //
            << "}\n";
}

void WriteEntryToStderr::operator()(turbo::LogSeverity severity,
                                    turbo::string_piece filename,
                                    turbo::string_piece log_message) const {
  if (!message.empty()) std::cerr << message << "\n";
  const std::string source_filename = turbo::CHexEscape(filename);
  const std::string text_message = turbo::CHexEscape(log_message);
  std::cerr << "LogEntry{\n"                                         //
            << "  source_filename: \"" << source_filename << "\"\n"  //
            << "  log_severity: " << severity << "\n"                //
            << "  text_message: \"" << text_message << "\"\n"        //
            << "}\n";
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
