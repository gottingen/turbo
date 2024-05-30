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

#include <turbo/strings/internal/str_format/bind.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <ios>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <turbo/base/config.h>
#include <turbo/base/optimization.h>
#include <turbo/strings/internal/str_format/arg.h>
#include <turbo/strings/internal/str_format/constexpr_parser.h>
#include <turbo/strings/internal/str_format/extension.h>
#include <turbo/strings/internal/str_format/output.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/span.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace str_format_internal {

namespace {

inline bool BindFromPosition(int position, int* value,
                             turbo::Span<const FormatArgImpl> pack) {
  assert(position > 0);
  if (static_cast<size_t>(position) > pack.size()) {
    return false;
  }
  // -1 because positions are 1-based
  return FormatArgImplFriend::ToInt(pack[static_cast<size_t>(position) - 1],
                                    value);
}

class ArgContext {
 public:
  explicit ArgContext(turbo::Span<const FormatArgImpl> pack) : pack_(pack) {}

  // Fill 'bound' with the results of applying the context's argument pack
  // to the specified 'unbound'. We synthesize a BoundConversion by
  // lining up a UnboundConversion with a user argument. We also
  // resolve any '*' specifiers for width and precision, so after
  // this call, 'bound' has all the information it needs to be formatted.
  // Returns false on failure.
  bool Bind(const UnboundConversion* unbound, BoundConversion* bound);

 private:
  turbo::Span<const FormatArgImpl> pack_;
};

inline bool ArgContext::Bind(const UnboundConversion* unbound,
                             BoundConversion* bound) {
  const FormatArgImpl* arg = nullptr;
  int arg_position = unbound->arg_position;
  if (static_cast<size_t>(arg_position - 1) >= pack_.size()) return false;
  arg = &pack_[static_cast<size_t>(arg_position - 1)];  // 1-based

  if (unbound->flags != Flags::kBasic) {
    int width = unbound->width.value();
    bool force_left = false;
    if (unbound->width.is_from_arg()) {
      if (!BindFromPosition(unbound->width.get_from_arg(), &width, pack_))
        return false;
      if (width < 0) {
        // "A negative field width is taken as a '-' flag followed by a
        // positive field width."
        force_left = true;
        // Make sure we don't overflow the width when negating it.
        width = -std::max(width, -std::numeric_limits<int>::max());
      }
    }

    int precision = unbound->precision.value();
    if (unbound->precision.is_from_arg()) {
      if (!BindFromPosition(unbound->precision.get_from_arg(), &precision,
                            pack_))
        return false;
    }

    FormatConversionSpecImplFriend::SetWidth(width, bound);
    FormatConversionSpecImplFriend::SetPrecision(precision, bound);

    if (force_left) {
      FormatConversionSpecImplFriend::SetFlags(unbound->flags | Flags::kLeft,
                                               bound);
    } else {
      FormatConversionSpecImplFriend::SetFlags(unbound->flags, bound);
    }

    FormatConversionSpecImplFriend::SetLengthMod(unbound->length_mod, bound);
  } else {
    FormatConversionSpecImplFriend::SetFlags(unbound->flags, bound);
    FormatConversionSpecImplFriend::SetWidth(-1, bound);
    FormatConversionSpecImplFriend::SetPrecision(-1, bound);
  }
  FormatConversionSpecImplFriend::SetConversionChar(unbound->conv, bound);
  bound->set_arg(arg);
  return true;
}

template <typename Converter>
class ConverterConsumer {
 public:
  ConverterConsumer(Converter converter, turbo::Span<const FormatArgImpl> pack)
      : converter_(converter), arg_context_(pack) {}

  bool Append(string_view s) {
    converter_.Append(s);
    return true;
  }
  bool ConvertOne(const UnboundConversion& conv, string_view conv_string) {
    BoundConversion bound;
    if (!arg_context_.Bind(&conv, &bound)) return false;
    return converter_.ConvertOne(bound, conv_string);
  }

 private:
  Converter converter_;
  ArgContext arg_context_;
};

template <typename Converter>
bool ConvertAll(const UntypedFormatSpecImpl format,
                turbo::Span<const FormatArgImpl> args, Converter converter) {
  if (format.has_parsed_conversion()) {
    return format.parsed_conversion()->ProcessFormat(
        ConverterConsumer<Converter>(converter, args));
  } else {
    return ParseFormatString(format.str(),
                             ConverterConsumer<Converter>(converter, args));
  }
}

class DefaultConverter {
 public:
  explicit DefaultConverter(FormatSinkImpl* sink) : sink_(sink) {}

  void Append(string_view s) const { sink_->Append(s); }

  bool ConvertOne(const BoundConversion& bound, string_view /*conv*/) const {
    return FormatArgImplFriend::Convert(*bound.arg(), bound, sink_);
  }

 private:
  FormatSinkImpl* sink_;
};

class SummarizingConverter {
 public:
  explicit SummarizingConverter(FormatSinkImpl* sink) : sink_(sink) {}

  void Append(string_view s) const { sink_->Append(s); }

  bool ConvertOne(const BoundConversion& bound, string_view /*conv*/) const {
    UntypedFormatSpecImpl spec("%d");

    std::ostringstream ss;
    ss << "{" << Streamable(spec, {*bound.arg()}) << ":"
       << FormatConversionSpecImplFriend::FlagsToString(bound);
    if (bound.width() >= 0) ss << bound.width();
    if (bound.precision() >= 0) ss << "." << bound.precision();
    ss << bound.conversion_char() << "}";
    Append(ss.str());
    return true;
  }

 private:
  FormatSinkImpl* sink_;
};

}  // namespace

bool BindWithPack(const UnboundConversion* props,
                  turbo::Span<const FormatArgImpl> pack,
                  BoundConversion* bound) {
  return ArgContext(pack).Bind(props, bound);
}

std::string Summarize(const UntypedFormatSpecImpl format,
                      turbo::Span<const FormatArgImpl> args) {
  typedef SummarizingConverter Converter;
  std::string out;
  {
    // inner block to destroy sink before returning out. It ensures a last
    // flush.
    FormatSinkImpl sink(&out);
    if (!ConvertAll(format, args, Converter(&sink))) {
      return "";
    }
  }
  return out;
}

bool format_untyped(FormatRawSinkImpl raw_sink,
                   const UntypedFormatSpecImpl format,
                   turbo::Span<const FormatArgImpl> args) {
  FormatSinkImpl sink(raw_sink);
  using Converter = DefaultConverter;
  return ConvertAll(format, args, Converter(&sink));
}

std::ostream& Streamable::Print(std::ostream& os) const {
  if (!format_untyped(&os, format_, args_)) os.setstate(std::ios::failbit);
  return os;
}

std::string& AppendPack(std::string* out, const UntypedFormatSpecImpl format,
                        turbo::Span<const FormatArgImpl> args) {
  size_t orig = out->size();
  if (TURBO_PREDICT_FALSE(!format_untyped(out, format, args))) {
    out->erase(orig);
  }
  return *out;
}

std::string FormatPack(UntypedFormatSpecImpl format,
                       turbo::Span<const FormatArgImpl> args) {
  std::string out;
  if (TURBO_PREDICT_FALSE(!format_untyped(&out, format, args))) {
    out.clear();
  }
  return out;
}

int FprintF(std::FILE* output, const UntypedFormatSpecImpl format,
            turbo::Span<const FormatArgImpl> args) {
  FILERawSink sink(output);
  if (!format_untyped(&sink, format, args)) {
    errno = EINVAL;
    return -1;
  }
  if (sink.error()) {
    errno = sink.error();
    return -1;
  }
  if (sink.count() > static_cast<size_t>(std::numeric_limits<int>::max())) {
    errno = EFBIG;
    return -1;
  }
  return static_cast<int>(sink.count());
}

int SnprintF(char* output, size_t size, const UntypedFormatSpecImpl format,
             turbo::Span<const FormatArgImpl> args) {
  BufferRawSink sink(output, size ? size - 1 : 0);
  if (!format_untyped(&sink, format, args)) {
    errno = EINVAL;
    return -1;
  }
  size_t total = sink.total_written();
  if (size) output[std::min(total, size - 1)] = 0;
  return static_cast<int>(total);
}

}  // namespace str_format_internal
TURBO_NAMESPACE_END
}  // namespace turbo
