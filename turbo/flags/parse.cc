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

#include "turbo/flags/parse.h"

#include <stdlib.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "turbo/meta/container.h"
#include "turbo/platform/attributes.h"
#include "turbo/platform/config.h"
#include "turbo/platform/const_init.h"
#include "turbo/platform/thread_annotations.h"
#include "turbo/flags/commandlineflag.h"
#include "turbo/flags/config.h"
#include "turbo/flags/flag.h"
#include "turbo/flags/internal/commandlineflag.h"
#include "turbo/flags/internal/flag.h"
#include "turbo/flags/internal/parse.h"
#include "turbo/flags/internal/private_handle_accessor.h"
#include "turbo/flags/internal/program_name.h"
#include "turbo/flags/internal/usage.h"
#include "turbo/flags/reflection.h"
#include "turbo/flags/usage.h"
#include "turbo/flags/usage_config.h"
#include "turbo/strings/ascii.h"
#include "turbo/strings/internal/damerau_levenshtein_distance.h"
#include "turbo/strings/str_cat.h"
#include "turbo/strings/str_join.h"
#include "turbo/strings/string_view.h"
#include "turbo/strings/strip.h"
#include "turbo/synchronization/mutex.h"

// --------------------------------------------------------------------

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {
namespace {

TURBO_CONST_INIT turbo::Mutex processing_checks_guard(turbo::kConstInit);

TURBO_CONST_INIT bool flagfile_needs_processing
    TURBO_GUARDED_BY(processing_checks_guard) = false;
TURBO_CONST_INIT bool fromenv_needs_processing
    TURBO_GUARDED_BY(processing_checks_guard) = false;
TURBO_CONST_INIT bool tryfromenv_needs_processing
    TURBO_GUARDED_BY(processing_checks_guard) = false;

TURBO_CONST_INIT turbo::Mutex specified_flags_guard(turbo::kConstInit);
TURBO_CONST_INIT std::vector<const CommandLineFlag*>* specified_flags
    TURBO_GUARDED_BY(specified_flags_guard) = nullptr;

// Suggesting at most kMaxHints flags in case of misspellings.
TURBO_CONST_INIT const size_t kMaxHints = 100;
// Suggesting only flags which have a smaller distance than kMaxDistance.
TURBO_CONST_INIT const size_t kMaxDistance = 3;

struct SpecifiedFlagsCompare {
  bool operator()(const CommandLineFlag* a, const CommandLineFlag* b) const {
    return a->Name() < b->Name();
  }
  bool operator()(const CommandLineFlag* a, std::string_view b) const {
    return a->Name() < b;
  }
  bool operator()(std::string_view a, const CommandLineFlag* b) const {
    return a < b->Name();
  }
};

}  // namespace
}  // namespace flags_internal
TURBO_NAMESPACE_END
}  // namespace turbo

TURBO_FLAG(std::vector<std::string>, flagfile, {},
          "comma-separated list of files to load flags from")
    .OnUpdate([]() {
      if (turbo::GetFlag(FLAGS_flagfile).empty()) return;

      turbo::MutexLock l(&turbo::flags_internal::processing_checks_guard);

      // Setting this flag twice before it is handled most likely an internal
      // error and should be reviewed by developers.
      if (turbo::flags_internal::flagfile_needs_processing) {
        TURBO_INTERNAL_LOG(WARNING, "flagfile set twice before it is handled");
      }

      turbo::flags_internal::flagfile_needs_processing = true;
    });
TURBO_FLAG(std::vector<std::string>, fromenv, {},
          "comma-separated list of flags to set from the environment"
          " [use 'export FLAGS_flag1=value']")
    .OnUpdate([]() {
      if (turbo::GetFlag(FLAGS_fromenv).empty()) return;

      turbo::MutexLock l(&turbo::flags_internal::processing_checks_guard);

      // Setting this flag twice before it is handled most likely an internal
      // error and should be reviewed by developers.
      if (turbo::flags_internal::fromenv_needs_processing) {
        TURBO_INTERNAL_LOG(WARNING, "fromenv set twice before it is handled.");
      }

      turbo::flags_internal::fromenv_needs_processing = true;
    });
TURBO_FLAG(std::vector<std::string>, tryfromenv, {},
          "comma-separated list of flags to try to set from the environment if "
          "present")
    .OnUpdate([]() {
      if (turbo::GetFlag(FLAGS_tryfromenv).empty()) return;

      turbo::MutexLock l(&turbo::flags_internal::processing_checks_guard);

      // Setting this flag twice before it is handled most likely an internal
      // error and should be reviewed by developers.
      if (turbo::flags_internal::tryfromenv_needs_processing) {
        TURBO_INTERNAL_LOG(WARNING,
                          "tryfromenv set twice before it is handled.");
      }

      turbo::flags_internal::tryfromenv_needs_processing = true;
    });

TURBO_FLAG(std::vector<std::string>, undefok, {},
          "comma-separated list of flag names that it is okay to specify "
          "on the command line even if the program does not define a flag "
          "with that name");

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {

namespace {

class ArgsList {
 public:
  ArgsList() : next_arg_(0) {}
  ArgsList(int argc, char* argv[]) : args_(argv, argv + argc), next_arg_(0) {}
  explicit ArgsList(const std::vector<std::string>& args)
      : args_(args), next_arg_(0) {}

  // Returns success status: true if parsing successful, false otherwise.
  bool ReadFromFlagfile(const std::string& flag_file_name);

  size_t Size() const { return args_.size() - next_arg_; }
  size_t FrontIndex() const { return next_arg_; }
  std::string_view Front() const { return args_[next_arg_]; }
  void PopFront() { next_arg_++; }

 private:
  std::vector<std::string> args_;
  size_t next_arg_;
};

bool ArgsList::ReadFromFlagfile(const std::string& flag_file_name) {
  std::ifstream flag_file(flag_file_name);

  if (!flag_file) {
    flags_internal::ReportUsageError(
        turbo::StrCat("Can't open flagfile ", flag_file_name), true);

    return false;
  }

  // This argument represents fake argv[0], which should be present in all arg
  // lists.
  args_.push_back("");

  std::string line;
  bool success = true;

  while (std::getline(flag_file, line)) {
    std::string_view stripped = turbo::StripLeadingAsciiWhitespace(line);

    if (stripped.empty() || stripped[0] == '#') {
      // Comment or empty line; just ignore.
      continue;
    }

    if (stripped[0] == '-') {
      if (stripped == "--") {
        flags_internal::ReportUsageError(
            "Flagfile can't contain position arguments or --", true);

        success = false;
        break;
      }

      args_.push_back(std::string(stripped));
      continue;
    }

    flags_internal::ReportUsageError(
        turbo::StrCat("Unexpected line in the flagfile ", flag_file_name, ": ",
                     line),
        true);

    success = false;
  }

  return success;
}

// --------------------------------------------------------------------

// Reads the environment variable with name `name` and stores results in
// `value`. If variable is not present in environment returns false, otherwise
// returns true.
bool GetEnvVar(const char* var_name, std::string& var_value) {
#ifdef _WIN32
  char buf[1024];
  auto get_res = GetEnvironmentVariableA(var_name, buf, sizeof(buf));
  if (get_res >= sizeof(buf)) {
    return false;
  }

  if (get_res == 0) {
    return false;
  }

  var_value = std::string(buf, get_res);
#else
  const char* val = ::getenv(var_name);
  if (val == nullptr) {
    return false;
  }

  var_value = val;
#endif

  return true;
}

// --------------------------------------------------------------------

// Returns:
//  Flag name or empty if arg= --
//  Flag value after = in --flag=value (empty if --foo)
//  "Is empty value" status. True if arg= --foo=, false otherwise. This is
//  required to separate --foo from --foo=.
// For example:
//      arg           return values
//   "--foo=bar" -> {"foo", "bar", false}.
//   "--foo"     -> {"foo", "", false}.
//   "--foo="    -> {"foo", "", true}.
std::tuple<std::string_view, std::string_view, bool> SplitNameAndValue(
    std::string_view arg) {
  // Allow -foo and --foo
  turbo::ConsumePrefix(&arg, "-");

  if (arg.empty()) {
    return std::make_tuple("", "", false);
  }

  auto equal_sign_pos = arg.find("=");

  std::string_view flag_name = arg.substr(0, equal_sign_pos);

  std::string_view value;
  bool is_empty_value = false;

  if (equal_sign_pos != std::string_view::npos) {
    value = arg.substr(equal_sign_pos + 1);
    is_empty_value = value.empty();
  }

  return std::make_tuple(flag_name, value, is_empty_value);
}

// --------------------------------------------------------------------

// Returns:
//  found flag or nullptr
//  is negative in case of --nofoo
std::tuple<CommandLineFlag*, bool> LocateFlag(std::string_view flag_name) {
  CommandLineFlag* flag = turbo::FindCommandLineFlag(flag_name);
  bool is_negative = false;

  if (!flag && turbo::ConsumePrefix(&flag_name, "no")) {
    flag = turbo::FindCommandLineFlag(flag_name);
    is_negative = true;
  }

  return std::make_tuple(flag, is_negative);
}

// --------------------------------------------------------------------

// Verify that default values of typed flags must be convertible to string and
// back.
void CheckDefaultValuesParsingRoundtrip() {
#ifndef NDEBUG
  flags_internal::ForEachFlag([&](CommandLineFlag& flag) {
    if (flag.IsRetired()) return;

#define TURBO_FLAGS_INTERNAL_IGNORE_TYPE(T, _) \
  if (flag.IsOfType<T>()) return;

    TURBO_FLAGS_INTERNAL_SUPPORTED_TYPES(TURBO_FLAGS_INTERNAL_IGNORE_TYPE)
#undef TURBO_FLAGS_INTERNAL_IGNORE_TYPE

    flags_internal::PrivateHandleAccessor::CheckDefaultValueParsingRoundtrip(
        flag);
  });
#endif
}

// --------------------------------------------------------------------

// Returns success status, which is true if we successfully read all flag files,
// in which case new ArgLists are appended to the input_args in a reverse order
// of file names in the input flagfiles list. This order ensures that flags from
// the first flagfile in the input list are processed before the second flagfile
// etc.
bool ReadFlagfiles(const std::vector<std::string>& flagfiles,
                   std::vector<ArgsList>& input_args) {
  bool success = true;
  for (auto it = flagfiles.rbegin(); it != flagfiles.rend(); ++it) {
    ArgsList al;

    if (al.ReadFromFlagfile(*it)) {
      input_args.push_back(al);
    } else {
      success = false;
    }
  }

  return success;
}

// Returns success status, which is true if were able to locate all environment
// variables correctly or if fail_on_absent_in_env is false. The environment
// variable names are expected to be of the form `FLAGS_<flag_name>`, where
// `flag_name` is a string from the input flag_names list. If successful we
// append a single ArgList at the end of the input_args.
bool ReadFlagsFromEnv(const std::vector<std::string>& flag_names,
                      std::vector<ArgsList>& input_args,
                      bool fail_on_absent_in_env) {
  bool success = true;
  std::vector<std::string> args;

  // This argument represents fake argv[0], which should be present in all arg
  // lists.
  args.push_back("");

  for (const auto& flag_name : flag_names) {
    // Avoid infinite recursion.
    if (flag_name == "fromenv" || flag_name == "tryfromenv") {
      flags_internal::ReportUsageError(
          turbo::StrCat("Infinite recursion on flag ", flag_name), true);

      success = false;
      continue;
    }

    const std::string envname = turbo::StrCat("FLAGS_", flag_name);
    std::string envval;
    if (!GetEnvVar(envname.c_str(), envval)) {
      if (fail_on_absent_in_env) {
        flags_internal::ReportUsageError(
            turbo::StrCat(envname, " not found in environment"), true);

        success = false;
      }

      continue;
    }

    args.push_back(turbo::StrCat("--", flag_name, "=", envval));
  }

  if (success) {
    input_args.emplace_back(args);
  }

  return success;
}

// --------------------------------------------------------------------

// Returns success status, which is true if were able to handle all generator
// flags (flagfile, fromenv, tryfromemv) successfully.
bool HandleGeneratorFlags(std::vector<ArgsList>& input_args,
                          std::vector<std::string>& flagfile_value) {
  bool success = true;

  turbo::MutexLock l(&flags_internal::processing_checks_guard);

  // flagfile could have been set either on a command line or
  // programmatically before invoking ParseCommandLine. Note that we do not
  // actually process arguments specified in the flagfile, but instead
  // create a secondary arguments list to be processed along with the rest
  // of the comamnd line arguments. Since we always the process most recently
  // created list of arguments first, this will result in flagfile argument
  // being processed before any other argument in the command line. If
  // FLAGS_flagfile contains more than one file name we create multiple new
  // levels of arguments in a reverse order of file names. Thus we always
  // process arguments from first file before arguments containing in a
  // second file, etc. If flagfile contains another
  // --flagfile inside of it, it will produce new level of arguments and
  // processed before the rest of the flagfile. We are also collecting all
  // flagfiles set on original command line. Unlike the rest of the flags,
  // this flag can be set multiple times and is expected to be handled
  // multiple times. We are collecting them all into a single list and set
  // the value of FLAGS_flagfile to that value at the end of the parsing.
  if (flags_internal::flagfile_needs_processing) {
    auto flagfiles = turbo::GetFlag(FLAGS_flagfile);

    if (input_args.size() == 1) {
      flagfile_value.insert(flagfile_value.end(), flagfiles.begin(),
                            flagfiles.end());
    }

    success &= ReadFlagfiles(flagfiles, input_args);

    flags_internal::flagfile_needs_processing = false;
  }

  // Similar to flagfile fromenv/tryfromemv can be set both
  // programmatically and at runtime on a command line. Unlike flagfile these
  // can't be recursive.
  if (flags_internal::fromenv_needs_processing) {
    auto flags_list = turbo::GetFlag(FLAGS_fromenv);

    success &= ReadFlagsFromEnv(flags_list, input_args, true);

    flags_internal::fromenv_needs_processing = false;
  }

  if (flags_internal::tryfromenv_needs_processing) {
    auto flags_list = turbo::GetFlag(FLAGS_tryfromenv);

    success &= ReadFlagsFromEnv(flags_list, input_args, false);

    flags_internal::tryfromenv_needs_processing = false;
  }

  return success;
}

// --------------------------------------------------------------------

void ResetGeneratorFlags(const std::vector<std::string>& flagfile_value) {
  // Setting flagfile to the value which collates all the values set on a
  // command line and programmatically. So if command line looked like
  // --flagfile=f1 --flagfile=f2 the final value of the FLAGS_flagfile flag is
  // going to be {"f1", "f2"}
  if (!flagfile_value.empty()) {
    turbo::SetFlag(&FLAGS_flagfile, flagfile_value);
    turbo::MutexLock l(&flags_internal::processing_checks_guard);
    flags_internal::flagfile_needs_processing = false;
  }

  // fromenv/tryfromenv are set to <undefined> value.
  if (!turbo::GetFlag(FLAGS_fromenv).empty()) {
    turbo::SetFlag(&FLAGS_fromenv, {});
  }
  if (!turbo::GetFlag(FLAGS_tryfromenv).empty()) {
    turbo::SetFlag(&FLAGS_tryfromenv, {});
  }

  turbo::MutexLock l(&flags_internal::processing_checks_guard);
  flags_internal::fromenv_needs_processing = false;
  flags_internal::tryfromenv_needs_processing = false;
}

// --------------------------------------------------------------------

// Returns:
//  success status
//  deduced value
// We are also mutating curr_list in case if we need to get a hold of next
// argument in the input.
std::tuple<bool, std::string_view> DeduceFlagValue(const CommandLineFlag& flag,
                                                    std::string_view value,
                                                    bool is_negative,
                                                    bool is_empty_value,
                                                    ArgsList* curr_list) {
  // Value is either an argument suffix after `=` in "--foo=<value>"
  // or separate argument in case of "--foo" "<value>".

  // boolean flags have these forms:
  //   --foo
  //   --nofoo
  //   --foo=true
  //   --foo=false
  //   --nofoo=<value> is not supported
  //   --foo <value> is not supported

  // non boolean flags have these forms:
  // --foo=<value>
  // --foo <value>
  // --nofoo is not supported

  if (flag.IsOfType<bool>()) {
    if (value.empty()) {
      if (is_empty_value) {
        // "--bool_flag=" case
        flags_internal::ReportUsageError(
            turbo::StrCat(
                "Missing the value after assignment for the boolean flag '",
                flag.Name(), "'"),
            true);
        return std::make_tuple(false, "");
      }

      // "--bool_flag" case
      value = is_negative ? "0" : "1";
    } else if (is_negative) {
      // "--nobool_flag=Y" case
      flags_internal::ReportUsageError(
          turbo::StrCat("Negative form with assignment is not valid for the "
                       "boolean flag '",
                       flag.Name(), "'"),
          true);
      return std::make_tuple(false, "");
    }
  } else if (is_negative) {
    // "--noint_flag=1" case
    flags_internal::ReportUsageError(
        turbo::StrCat("Negative form is not valid for the flag '", flag.Name(),
                     "'"),
        true);
    return std::make_tuple(false, "");
  } else if (value.empty() && (!is_empty_value)) {
    if (curr_list->Size() == 1) {
      // "--int_flag" case
      flags_internal::ReportUsageError(
          turbo::StrCat("Missing the value for the flag '", flag.Name(), "'"),
          true);
      return std::make_tuple(false, "");
    }

    // "--int_flag" "10" case
    curr_list->PopFront();
    value = curr_list->Front();

    // Heuristic to detect the case where someone treats a string arg
    // like a bool or just forgets to pass a value:
    // --my_string_var --foo=bar
    // We look for a flag of string type, whose value begins with a
    // dash and corresponds to known flag or standalone --.
    if (!value.empty() && value[0] == '-' && flag.IsOfType<std::string>()) {
      auto maybe_flag_name = std::get<0>(SplitNameAndValue(value.substr(1)));

      if (maybe_flag_name.empty() ||
          std::get<0>(LocateFlag(maybe_flag_name)) != nullptr) {
        // "--string_flag" "--known_flag" case
        TURBO_INTERNAL_LOG(
            WARNING,
            turbo::StrCat("Did you really mean to set flag '", flag.Name(),
                         "' to the value '", value, "'?"));
      }
    }
  }

  return std::make_tuple(true, value);
}

// --------------------------------------------------------------------

bool CanIgnoreUndefinedFlag(std::string_view flag_name) {
  auto undefok = turbo::GetFlag(FLAGS_undefok);
  if (std::find(undefok.begin(), undefok.end(), flag_name) != undefok.end()) {
    return true;
  }

  if (turbo::ConsumePrefix(&flag_name, "no") &&
      std::find(undefok.begin(), undefok.end(), flag_name) != undefok.end()) {
    return true;
  }

  return false;
}

}  // namespace

// --------------------------------------------------------------------

bool WasPresentOnCommandLine(std::string_view flag_name) {
  turbo::MutexLock l(&specified_flags_guard);
  TURBO_INTERNAL_CHECK(specified_flags != nullptr,
                      "ParseCommandLine is not invoked yet");

  return std::binary_search(specified_flags->begin(), specified_flags->end(),
                            flag_name, SpecifiedFlagsCompare{});
}

// --------------------------------------------------------------------

struct BestHints {
  explicit BestHints(uint8_t _max) : best_distance(_max + 1) {}
  bool AddHint(std::string_view hint, uint8_t distance) {
    if (hints.size() >= kMaxHints) return false;
    if (distance == best_distance) {
      hints.emplace_back(hint);
    }
    if (distance < best_distance) {
      best_distance = distance;
      hints = std::vector<std::string>{std::string(hint)};
    }
    return true;
  }

  uint8_t best_distance;
  std::vector<std::string> hints;
};

// Return the list of flags with the smallest Damerau-Levenshtein distance to
// the given flag.
std::vector<std::string> GetMisspellingHints(const std::string_view flag) {
  const size_t maxCutoff = std::min(flag.size() / 2 + 1, kMaxDistance);
  auto undefok = turbo::GetFlag(FLAGS_undefok);
  BestHints best_hints(static_cast<uint8_t>(maxCutoff));
  turbo::flags_internal::ForEachFlag([&](const CommandLineFlag& f) {
    if (best_hints.hints.size() >= kMaxHints) return;
    uint8_t distance = strings_internal::CappedDamerauLevenshteinDistance(
        flag, f.Name(), best_hints.best_distance);
    best_hints.AddHint(f.Name(), distance);
    // For boolean flags, also calculate distance to the negated form.
    if (f.IsOfType<bool>()) {
      const std::string negated_flag = turbo::StrCat("no", f.Name());
      distance = strings_internal::CappedDamerauLevenshteinDistance(
          flag, negated_flag, best_hints.best_distance);
      best_hints.AddHint(negated_flag, distance);
    }
  });
  // Finally calculate distance to flags in "undefok".
  turbo::c_for_each(undefok, [&](const std::string_view f) {
    if (best_hints.hints.size() >= kMaxHints) return;
    uint8_t distance = strings_internal::CappedDamerauLevenshteinDistance(
        flag, f, best_hints.best_distance);
    best_hints.AddHint(turbo::StrCat(f, " (undefok)"), distance);
  });
  return best_hints.hints;
}

// --------------------------------------------------------------------

std::vector<char*> ParseCommandLineImpl(int argc, char* argv[],
                                        ArgvListAction arg_list_act,
                                        UsageFlagsAction usage_flag_act,
                                        OnUndefinedFlag on_undef_flag) {
  TURBO_INTERNAL_CHECK(argc > 0, "Missing argv[0]");

  // Once parsing has started we will not have more flag registrations.
  // If we did, they would be missing during parsing, which is a problem on
  // itself.
  flags_internal::FinalizeRegistry();

  // This routine does not return anything since we abort on failure.
  CheckDefaultValuesParsingRoundtrip();

  std::vector<std::string> flagfile_value;

  std::vector<ArgsList> input_args;
  input_args.push_back(ArgsList(argc, argv));

  std::vector<char*> output_args;
  std::vector<char*> positional_args;
  output_args.reserve(static_cast<size_t>(argc));

  // This is the list of undefined flags. The element of the list is the pair
  // consisting of boolean indicating if flag came from command line (vs from
  // some flag file we've read) and flag name.
  // TODO(rogeeff): Eliminate the first element in the pair after cleanup.
  std::vector<std::pair<bool, std::string>> undefined_flag_names;

  // Set program invocation name if it is not set before.
  if (ProgramInvocationName() == "UNKNOWN") {
    flags_internal::SetProgramInvocationName(argv[0]);
  }
  output_args.push_back(argv[0]);

  turbo::MutexLock l(&specified_flags_guard);
  if (specified_flags == nullptr) {
    specified_flags = new std::vector<const CommandLineFlag*>;
  } else {
    specified_flags->clear();
  }

  // Iterate through the list of the input arguments. First level are arguments
  // originated from argc/argv. Following levels are arguments originated from
  // recursive parsing of flagfile(s).
  bool success = true;
  while (!input_args.empty()) {
    // 10. First we process the built-in generator flags.
    success &= HandleGeneratorFlags(input_args, flagfile_value);

    // 30. Select top-most (most recent) arguments list. If it is empty drop it
    // and re-try.
    ArgsList& curr_list = input_args.back();

    curr_list.PopFront();

    if (curr_list.Size() == 0) {
      input_args.pop_back();
      continue;
    }

    // 40. Pick up the front remaining argument in the current list. If current
    // stack of argument lists contains only one element - we are processing an
    // argument from the original argv.
    std::string_view arg(curr_list.Front());
    bool arg_from_argv = input_args.size() == 1;

    // 50. If argument does not start with - or is just "-" - this is
    // positional argument.
    if (!turbo::ConsumePrefix(&arg, "-") || arg.empty()) {
      TURBO_INTERNAL_CHECK(arg_from_argv,
                          "Flagfile cannot contain positional argument");

      positional_args.push_back(argv[curr_list.FrontIndex()]);
      continue;
    }

    if (arg_from_argv && (arg_list_act == ArgvListAction::kKeepParsedArgs)) {
      output_args.push_back(argv[curr_list.FrontIndex()]);
    }

    // 60. Split the current argument on '=' to figure out the argument
    // name and value. If flag name is empty it means we've got "--". value
    // can be empty either if there were no '=' in argument string at all or
    // an argument looked like "--foo=". In a latter case is_empty_value is
    // true.
    std::string_view flag_name;
    std::string_view value;
    bool is_empty_value = false;

    std::tie(flag_name, value, is_empty_value) = SplitNameAndValue(arg);

    // 70. "--" alone means what it does for GNU: stop flags parsing. We do
    // not support positional arguments in flagfiles, so we just drop them.
    if (flag_name.empty()) {
      TURBO_INTERNAL_CHECK(arg_from_argv,
                          "Flagfile cannot contain positional argument");

      curr_list.PopFront();
      break;
    }

    // 80. Locate the flag based on flag name. Handle both --foo and --nofoo
    CommandLineFlag* flag = nullptr;
    bool is_negative = false;
    std::tie(flag, is_negative) = LocateFlag(flag_name);

    if (flag == nullptr) {
      // Usage flags are not modeled as Turbo flags. Locate them separately.
      if (flags_internal::DeduceUsageFlags(flag_name, value)) {
        continue;
      }

      if (on_undef_flag != OnUndefinedFlag::kIgnoreUndefined) {
        undefined_flag_names.emplace_back(arg_from_argv,
                                          std::string(flag_name));
      }
      continue;
    }

    // 90. Deduce flag's value (from this or next argument)
    auto curr_index = curr_list.FrontIndex();
    bool value_success = true;
    std::tie(value_success, value) =
        DeduceFlagValue(*flag, value, is_negative, is_empty_value, &curr_list);
    success &= value_success;

    // If above call consumed an argument, it was a standalone value
    if (arg_from_argv && (arg_list_act == ArgvListAction::kKeepParsedArgs) &&
        (curr_index != curr_list.FrontIndex())) {
      output_args.push_back(argv[curr_list.FrontIndex()]);
    }

    // 100. Set the located flag to a new new value, unless it is retired.
    // Setting retired flag fails, but we ignoring it here while also reporting
    // access to retired flag.
    std::string error;
    if (!flags_internal::PrivateHandleAccessor::ParseFrom(
            *flag, value, SET_FLAGS_VALUE, kCommandLine, error)) {
      if (flag->IsRetired()) continue;

      flags_internal::ReportUsageError(error, true);
      success = false;
    } else {
      specified_flags->push_back(flag);
    }
  }

  for (const auto& flag_name : undefined_flag_names) {
    if (CanIgnoreUndefinedFlag(flag_name.second)) continue;
    // Verify if flag_name has the "no" already removed
    std::vector<std::string> flags;
    if (flag_name.first) flags = GetMisspellingHints(flag_name.second);
    if (flags.empty()) {
      flags_internal::ReportUsageError(
          turbo::StrCat("Unknown command line flag '", flag_name.second, "'"),
          true);
    } else {
      flags_internal::ReportUsageError(
          turbo::StrCat("Unknown command line flag '", flag_name.second,
                       "'. Did you mean: ", turbo::StrJoin(flags, ", "), " ?"),
          true);
    }

    success = false;
  }

#if TURBO_FLAGS_STRIP_NAMES
  if (!success) {
    flags_internal::ReportUsageError(
        "NOTE: command line flags are disabled in this build", true);
  }
#endif

  if (!success) {
    flags_internal::HandleUsageFlags(std::cout,
                                     ProgramUsageMessage());
    std::exit(1);
  }

  if (usage_flag_act == UsageFlagsAction::kHandleUsage) {
    int exit_code = flags_internal::HandleUsageFlags(
        std::cout, ProgramUsageMessage());

    if (exit_code != -1) {
      std::exit(exit_code);
    }
  }

  ResetGeneratorFlags(flagfile_value);

  // Reinstate positional args which were intermixed with flags in the arguments
  // list.
  for (auto arg : positional_args) {
    output_args.push_back(arg);
  }

  // All the remaining arguments are positional.
  if (!input_args.empty()) {
    for (size_t arg_index = input_args.back().FrontIndex();
         arg_index < static_cast<size_t>(argc); ++arg_index) {
      output_args.push_back(argv[arg_index]);
    }
  }

  // Trim and sort the vector.
  specified_flags->shrink_to_fit();
  std::sort(specified_flags->begin(), specified_flags->end(),
            SpecifiedFlagsCompare{});
  return output_args;
}

}  // namespace flags_internal

// --------------------------------------------------------------------

std::vector<char*> ParseCommandLine(int argc, char* argv[]) {
  return flags_internal::ParseCommandLineImpl(
      argc, argv, flags_internal::ArgvListAction::kRemoveParsedArgs,
      flags_internal::UsageFlagsAction::kHandleUsage,
      flags_internal::OnUndefinedFlag::kAbortIfUndefined);
}

TURBO_NAMESPACE_END
}  // namespace turbo
