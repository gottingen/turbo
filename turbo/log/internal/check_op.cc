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

#include <turbo/log/internal/check_op.h>

#include <string.h>

#include <ostream>

#include <turbo/strings/string_view.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h>  // for strcasecmp, but msvc does not have this header
#endif

#include <sstream>
#include <string>

#include <turbo/base/config.h>
#include <turbo/strings/str_cat.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

#define TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(x) \
  template std::string* MakeCheckOpString(x, x, const char*)
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(bool);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(int64_t);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(uint64_t);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(float);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(double);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(char);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(unsigned char);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(const std::string&);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(const std::string_view&);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(const char*);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(const signed char*);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(const unsigned char*);
TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING(const void*);
#undef TURBO_LOGGING_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING

CheckOpMessageBuilder::CheckOpMessageBuilder(const char* exprtext) {
  stream_ << exprtext << " (";
}

std::ostream& CheckOpMessageBuilder::ForVar2() {
  stream_ << " vs. ";
  return stream_;
}

std::string* CheckOpMessageBuilder::NewString() {
  stream_ << ")";
  return new std::string(stream_.str());
}

void MakeCheckOpValueString(std::ostream& os, const char v) {
  if (v >= 32 && v <= 126) {
    os << "'" << v << "'";
  } else {
    os << "char value " << int{v};
  }
}

void MakeCheckOpValueString(std::ostream& os, const signed char v) {
  if (v >= 32 && v <= 126) {
    os << "'" << v << "'";
  } else {
    os << "signed char value " << int{v};
  }
}

void MakeCheckOpValueString(std::ostream& os, const unsigned char v) {
  if (v >= 32 && v <= 126) {
    os << "'" << v << "'";
  } else {
    os << "unsigned char value " << int{v};
  }
}

void MakeCheckOpValueString(std::ostream& os, const void* p) {
  if (p == nullptr) {
    os << "(null)";
  } else {
    os << p;
  }
}

// Helper functions for string comparisons.
#define DEFINE_CHECK_STROP_IMPL(name, func, expected)                      \
  std::string* Check##func##expected##Impl(const char* s1, const char* s2, \
                                           const char* exprtext) {         \
    bool equal = s1 == s2 || (s1 && s2 && !func(s1, s2));                  \
    if (equal == expected) {                                               \
      return nullptr;                                                      \
    } else {                                                               \
      return new std::string(                                              \
          turbo::str_cat(exprtext, " (", s1, " vs. ", s2, ")"));             \
    }                                                                      \
  }
DEFINE_CHECK_STROP_IMPL(CHECK_STREQ, strcmp, true)
DEFINE_CHECK_STROP_IMPL(CHECK_STRNE, strcmp, false)
DEFINE_CHECK_STROP_IMPL(CHECK_STRCASEEQ, strcasecmp, true)
DEFINE_CHECK_STROP_IMPL(CHECK_STRCASENE, strcasecmp, false)
#undef DEFINE_CHECK_STROP_IMPL

namespace detect_specialization {

StringifySink::StringifySink(std::ostream& os) : os_(os) {}

void StringifySink::Append(std::string_view text) { os_ << text; }

void StringifySink::Append(size_t length, char ch) {
  for (size_t i = 0; i < length; ++i) os_.put(ch);
}

void turbo_format_flush(StringifySink* sink, std::string_view text) {
  sink->Append(text);
}

}  // namespace detect_specialization

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
