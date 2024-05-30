//
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

#include <tests/log/test_actions.h>

#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/strings/escaping.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>
#include <turbo/times/time.h>

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
                                    turbo::string_view filename,
                                    turbo::string_view log_message) const {
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
