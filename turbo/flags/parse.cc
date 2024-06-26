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

#include <turbo/flags/parse.h>

#include <stdlib.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include <turbo/algorithm/container.h>
#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/const_init.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/flags/commandlineflag.h>
#include <turbo/flags/config.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/internal/commandlineflag.h>
#include <turbo/flags/internal/flag.h>
#include <turbo/flags/internal/parse.h>
#include <turbo/flags/internal/private_handle_accessor.h>
#include <turbo/flags/internal/program_name.h>
#include <turbo/flags/internal/usage.h>
#include <turbo/flags/reflection.h>
#include <turbo/flags/usage.h>
#include <turbo/flags/usage_config.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/internal/damerau_levenshtein_distance.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_join.h>
#include <turbo/strings/string_view.h>
#include <turbo/strings/strip.h>
#include <turbo/synchronization/mutex.h>

// --------------------------------------------------------------------

namespace turbo::flags_internal {
    namespace {

        TURBO_CONST_INIT turbo::Mutex processing_checks_guard(turbo::kConstInit);

        TURBO_CONST_INIT bool flagfile_needs_processing
                TURBO_GUARDED_BY(processing_checks_guard) = false;
        TURBO_CONST_INIT bool fromenv_needs_processing
                TURBO_GUARDED_BY(processing_checks_guard) = false;
        TURBO_CONST_INIT bool tryfromenv_needs_processing
                TURBO_GUARDED_BY(processing_checks_guard) = false;

        TURBO_CONST_INIT turbo::Mutex specified_flags_guard(turbo::kConstInit);
        TURBO_CONST_INIT std::vector<const CommandLineFlag *> *specified_flags
                TURBO_GUARDED_BY(specified_flags_guard) = nullptr;

        // Suggesting at most kMaxHints flags in case of misspellings.
        TURBO_CONST_INIT const size_t kMaxHints = 100;
        // Suggesting only flags which have a smaller distance than kMaxDistance.
        TURBO_CONST_INIT const size_t kMaxDistance = 3;

        struct SpecifiedFlagsCompare {
            bool operator()(const CommandLineFlag *a, const CommandLineFlag *b) const {
                return a->name() < b->name();
            }

            bool operator()(const CommandLineFlag *a, std::string_view b) const {
                return a->name() < b;
            }

            bool operator()(std::string_view a, const CommandLineFlag *b) const {
                return a < b->name();
            }
        };

    }  // namespace
}  // namespace turbo::flags_internal

// These flags influence how command line flags are parsed and are only intended
// to be set on the command line.  Avoid reading or setting them from C++ code.
TURBO_FLAG(std::vector<std::string>, flags_file, {},
           "comma-separated list of files to load flags from")
.on_update([]() noexcept {
    if (turbo::get_flag(FLAGS_flags_file).empty()) return;

    turbo::MutexLock l(&turbo::flags_internal::processing_checks_guard);

    // Setting this flag twice before it is handled most likely an internal
    // error and should be reviewed by developers.
    if (turbo::flags_internal::flagfile_needs_processing) {
        TURBO_INTERNAL_LOG(WARNING, "flags_file set twice before it is handled");
    }

    turbo::flags_internal::flagfile_needs_processing = true;
});
TURBO_FLAG(std::vector<std::string>, from_env, {},
           "comma-separated list of flags to set from the environment"
           " [use 'export FLAGS_flag1=value']")
.on_update([]() noexcept {
    if (turbo::get_flag(FLAGS_from_env).empty()) return;

    turbo::MutexLock l(&turbo::flags_internal::processing_checks_guard);

    // Setting this flag twice before it is handled most likely an internal
    // error and should be reviewed by developers.
    if (turbo::flags_internal::fromenv_needs_processing) {
        TURBO_INTERNAL_LOG(WARNING, "from_env set twice before it is handled.");
    }

    turbo::flags_internal::fromenv_needs_processing = true;
});
TURBO_FLAG(std::vector<std::string>, try_from_env, {},
           "comma-separated list of flags to try to set from the environment if "
           "present")
.on_update([]() noexcept {
    if (turbo::get_flag(FLAGS_try_from_env).empty()) return;

    turbo::MutexLock l(&turbo::flags_internal::processing_checks_guard);

    // Setting this flag twice before it is handled most likely an internal
    // error and should be reviewed by developers.
    if (turbo::flags_internal::tryfromenv_needs_processing) {
        TURBO_INTERNAL_LOG(WARNING,
                           "try_from_env set twice before it is handled.");
    }

    turbo::flags_internal::tryfromenv_needs_processing = true;
});

// Rather than reading or setting --undef_ok from C++ code, please consider using
// TURBO_RETIRED_FLAG instead.
TURBO_FLAG(std::vector<std::string>, undef_ok, {},
           "comma-separated list of flag names that it is okay to specify "
           "on the command line even if the program does not define a flag "
           "with that name");

namespace turbo::flags_internal {
    namespace {

        class ArgsList {
        public:
            ArgsList() : next_arg_(0) {}

            ArgsList(int argc, char *argv[]) : args_(argv, argv + argc), next_arg_(0) {}

            explicit ArgsList(const std::vector<std::string> &args)
                    : args_(args), next_arg_(0) {}

            // Returns success status: true if parsing successful, false otherwise.
            bool ReadFromFlagfile(const std::string &flag_file_name);

            size_t Size() const { return args_.size() - next_arg_; }

            size_t FrontIndex() const { return next_arg_; }

            std::string_view Front() const { return args_[next_arg_]; }

            void PopFront() { next_arg_++; }

        private:
            std::vector<std::string> args_;
            size_t next_arg_;
        };

        bool ArgsList::ReadFromFlagfile(const std::string &flag_file_name) {
            std::ifstream flag_file(flag_file_name);

            if (!flag_file) {
                flags_internal::ReportUsageError(
                        turbo::str_cat("Can't open flags_file ", flag_file_name), true);

                return false;
            }

            // This argument represents fake argv[0], which should be present in all arg
            // lists.
            args_.emplace_back("");

            std::string line;
            bool success = true;

            while (std::getline(flag_file, line)) {
                std::string_view stripped = turbo::trim_left(line);

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

                    args_.emplace_back(stripped);
                    continue;
                }

                flags_internal::ReportUsageError(
                        turbo::str_cat("Unexpected line in the flags_file ", flag_file_name, ": ",
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
        bool GetEnvVar(const char *var_name, std::string &var_value) {
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
            const char *val = ::getenv(var_name);
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
            turbo::consume_prefix(&arg, "-");

            if (arg.empty()) {
                return std::make_tuple("", "", false);
            }

            auto equal_sign_pos = arg.find('=');

            std::string_view flag_name = arg.substr(0, equal_sign_pos);

            std::string_view value;
            bool is_empty_value = false;

            if (equal_sign_pos != std::string_view::npos) {
                value = arg.substr(equal_sign_pos + 1);
                is_empty_value = value.empty();
            }

            return std::make_tuple(flag_name, value, is_empty_value);
        }

        // Returns:
        //  found flag or nullptr
        //  is negative in case of --nofoo
        std::tuple<CommandLineFlag *, bool> LocateFlag(std::string_view flag_name) {
            CommandLineFlag *flag = turbo::find_command_line_flag(flag_name);
            bool is_negative = false;

            if (!flag && turbo::consume_prefix(&flag_name, "no")) {
                flag = turbo::find_command_line_flag(flag_name);
                is_negative = true;
            }

            return std::make_tuple(flag, is_negative);
        }

        // --------------------------------------------------------------------

        // Verify that default values of typed flags must be convertible to string and
        // back.
        void CheckDefaultValuesParsingRoundtrip() {
#ifndef NDEBUG
            flags_internal::ForEachFlag([&](CommandLineFlag &flag) {
                if (flag.is_retired()) return;

#define TURBO_FLAGS_INTERNAL_IGNORE_TYPE(T, _) \
  if (flag.is_of_type<T>()) return;

                TURBO_FLAGS_INTERNAL_SUPPORTED_TYPES(TURBO_FLAGS_INTERNAL_IGNORE_TYPE)
#undef TURBO_FLAGS_INTERNAL_IGNORE_TYPE

                flags_internal::PrivateHandleAccessor::check_default_value_parsing_roundtrip(
                        flag);
            });
#endif
        }

        // --------------------------------------------------------------------

        // Returns success status, which is true if we successfully read all flag files,
        // in which case new ArgLists are appended to the input_args in a reverse order
        // of file names in the input flagfiles list. This order ensures that flags from
        // the first flags_file in the input list are processed before the second flags_file
        // etc.
        bool ReadFlagfiles(const std::vector<std::string> &flagfiles,
                           std::vector<ArgsList> &input_args) {
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
        bool ReadFlagsFromEnv(const std::vector<std::string> &flag_names,
                              std::vector<ArgsList> &input_args,
                              bool fail_on_absent_in_env) {
            bool success = true;
            std::vector<std::string> args;

            // This argument represents fake argv[0], which should be present in all arg
            // lists.
            args.emplace_back("");

            for (const auto &flag_name: flag_names) {
                // Avoid infinite recursion.
                if (flag_name == "from_env" || flag_name == "try_from_env") {
                    flags_internal::ReportUsageError(
                            turbo::str_cat("Infinite recursion on flag ", flag_name), true);

                    success = false;
                    continue;
                }

                const std::string envname = turbo::str_cat("FLAGS_", flag_name);
                std::string envval;
                if (!GetEnvVar(envname.c_str(), envval)) {
                    if (fail_on_absent_in_env) {
                        flags_internal::ReportUsageError(
                                turbo::str_cat(envname, " not found in environment"), true);

                        success = false;
                    }

                    continue;
                }

                args.push_back(turbo::str_cat("--", flag_name, "=", envval));
            }

            if (success) {
                input_args.emplace_back(args);
            }

            return success;
        }

        // Returns success status, which is true if were able to handle all generator
        // flags (flags_file, from_env, tryfromemv) successfully.
        bool HandleGeneratorFlags(std::vector<ArgsList> &input_args,
                                  std::vector<std::string> &flagfile_value) {
            bool success = true;

            turbo::MutexLock l(&flags_internal::processing_checks_guard);

            // flags_file could have been set either on a command line or
            // programmatically before invoking parse_command_line. Note that we do not
            // actually process arguments specified in the flags_file, but instead
            // create a secondary arguments list to be processed along with the rest
            // of the command line arguments. Since we always the process most recently
            // created list of arguments first, this will result in flags_file argument
            // being processed before any other argument in the command line. If
            // FLAGS_flagfile contains more than one file name we create multiple new
            // levels of arguments in a reverse order of file names. Thus we always
            // process arguments from first file before arguments containing in a
            // second file, etc. If flags_file contains another
            // --flags_file inside of it, it will produce new level of arguments and
            // processed before the rest of the flags_file. We are also collecting all
            // flagfiles set on original command line. Unlike the rest of the flags,
            // this flag can be set multiple times and is expected to be handled
            // multiple times. We are collecting them all into a single list and set
            // the value of FLAGS_flags_file to that value at the end of the parsing.
            if (flags_internal::flagfile_needs_processing) {
                auto flagfiles = turbo::get_flag(FLAGS_flags_file);

                if (input_args.size() == 1) {
                    flagfile_value.insert(flagfile_value.end(), flagfiles.begin(),
                                          flagfiles.end());
                }

                success &= ReadFlagfiles(flagfiles, input_args);

                flags_internal::flagfile_needs_processing = false;
            }

            // Similar to flags_file from_env/tryfromemv can be set both
            // programmatically and at runtime on a command line. Unlike flags_file these
            // can't be recursive.
            if (flags_internal::fromenv_needs_processing) {
                auto flags_list = turbo::get_flag(FLAGS_from_env);

                success &= ReadFlagsFromEnv(flags_list, input_args, true);

                flags_internal::fromenv_needs_processing = false;
            }

            if (flags_internal::tryfromenv_needs_processing) {
                auto flags_list = turbo::get_flag(FLAGS_try_from_env);

                success &= ReadFlagsFromEnv(flags_list, input_args, false);

                flags_internal::tryfromenv_needs_processing = false;
            }

            return success;
        }


        void ResetGeneratorFlags(const std::vector<std::string> &flagfile_value) {
            // Setting flags_file to the value which collates all the values set on a
            // command line and programmatically. So if command line looked like
            // --flags_file=f1 --flags_file=f2 the final value of the FLAGS_flags_file flag is
            // going to be {"f1", "f2"}
            if (!flagfile_value.empty()) {
                turbo::set_flag(&FLAGS_flags_file, flagfile_value);
                turbo::MutexLock l(&flags_internal::processing_checks_guard);
                flags_internal::flagfile_needs_processing = false;
            }

            // from_env/try_from_env are set to <undefined> value.
            if (!turbo::get_flag(FLAGS_from_env).empty()) {
                turbo::set_flag(&FLAGS_from_env, {});
            }
            if (!turbo::get_flag(FLAGS_try_from_env).empty()) {
                turbo::set_flag(&FLAGS_try_from_env, {});
            }

            turbo::MutexLock l(&flags_internal::processing_checks_guard);
            flags_internal::fromenv_needs_processing = false;
            flags_internal::tryfromenv_needs_processing = false;
        }


        // Returns:
        //  success status
        //  deduced value
        // We are also mutating curr_list in case if we need to get a hold of next
        // argument in the input.
        std::tuple<bool, std::string_view> DeduceFlagValue(const CommandLineFlag &flag,
                                                             std::string_view value,
                                                             bool is_negative,
                                                             bool is_empty_value,
                                                             ArgsList *curr_list) {
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

            if (flag.is_of_type<bool>()) {
                if (value.empty()) {
                    if (is_empty_value) {
                        // "--bool_flag=" case
                        flags_internal::ReportUsageError(
                                turbo::str_cat(
                                        "Missing the value after assignment for the boolean flag '",
                                        flag.name(), "'"),
                                true);
                        return std::make_tuple(false, "");
                    }

                    // "--bool_flag" case
                    value = is_negative ? "0" : "1";
                } else if (is_negative) {
                    // "--nobool_flag=Y" case
                    flags_internal::ReportUsageError(
                            turbo::str_cat("Negative form with assignment is not valid for the "
                                           "boolean flag '",
                                           flag.name(), "'"),
                            true);
                    return std::make_tuple(false, "");
                }
            } else if (is_negative) {
                // "--noint_flag=1" case
                flags_internal::ReportUsageError(
                        turbo::str_cat("Negative form is not valid for the flag '", flag.name(),
                                       "'"),
                        true);
                return std::make_tuple(false, "");
            } else if (value.empty() && (!is_empty_value)) {
                if (curr_list->Size() == 1) {
                    // "--int_flag" case
                    flags_internal::ReportUsageError(
                            turbo::str_cat("Missing the value for the flag '", flag.name(), "'"),
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
                if (!value.empty() && value[0] == '-' && flag.is_of_type<std::string>()) {
                    auto maybe_flag_name = std::get<0>(SplitNameAndValue(value.substr(1)));

                    if (maybe_flag_name.empty() ||
                        std::get<0>(LocateFlag(maybe_flag_name)) != nullptr) {
                        // "--string_flag" "--known_flag" case
                        TURBO_INTERNAL_LOG(
                                WARNING,
                                turbo::str_cat("Did you really mean to set flag '", flag.name(),
                                               "' to the value '", value, "'?"));
                    }
                }
            }

            return std::make_tuple(true, value);
        }


        bool CanIgnoreUndefinedFlag(std::string_view flag_name) {
            auto undef_ok = turbo::get_flag(FLAGS_undef_ok);
            if (std::find(undef_ok.begin(), undef_ok.end(), flag_name) != undef_ok.end()) {
                return true;
            }

            if (turbo::consume_prefix(&flag_name, "no") &&
                std::find(undef_ok.begin(), undef_ok.end(), flag_name) != undef_ok.end()) {
                return true;
            }

            return false;
        }


        void report_unrecognized_flags(
                const std::vector<UnrecognizedFlag> &unrecognized_flags,
                bool report_as_fatal_error) {
            for (const auto &unrecognized: unrecognized_flags) {
                // Verify if flag_name has the "no" already removed
                std::vector<std::string> misspelling_hints;
                if (unrecognized.source == UnrecognizedFlag::kFromArgv) {
                    misspelling_hints =
                            flags_internal::GetMisspellingHints(unrecognized.flag_name);
                }

                if (misspelling_hints.empty()) {
                    flags_internal::ReportUsageError(
                            turbo::str_cat("Unknown command line flag '", unrecognized.flag_name,
                                           "'"),
                            report_as_fatal_error);
                } else {
                    flags_internal::ReportUsageError(
                            turbo::str_cat("Unknown command line flag '", unrecognized.flag_name,
                                           "'. Did you mean: ",
                                           turbo::str_join(misspelling_hints, ", "), " ?"),
                            report_as_fatal_error);
                }
            }
        }

    }  // namespace

    // --------------------------------------------------------------------

    bool WasPresentOnCommandLine(std::string_view flag_name) {
        turbo::ReaderMutexLock l(&specified_flags_guard);
        TURBO_INTERNAL_CHECK(specified_flags != nullptr,
                             "parse_command_line is not invoked yet");

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
        auto undef_ok = turbo::get_flag(FLAGS_undef_ok);
        BestHints best_hints(static_cast<uint8_t>(maxCutoff));
        flags_internal::ForEachFlag([&](const CommandLineFlag &f) {
            if (best_hints.hints.size() >= kMaxHints) return;
            uint8_t distance = strings_internal::CappedDamerauLevenshteinDistance(
                    flag, f.name(), best_hints.best_distance);
            best_hints.AddHint(f.name(), distance);
            // For boolean flags, also calculate distance to the negated form.
            if (f.is_of_type<bool>()) {
                const std::string negated_flag = turbo::str_cat("no", f.name());
                distance = strings_internal::CappedDamerauLevenshteinDistance(
                        flag, negated_flag, best_hints.best_distance);
                best_hints.AddHint(negated_flag, distance);
            }
        });
        // Finally calculate distance to flags in "undef_ok".
        turbo::c_for_each(undef_ok, [&](const std::string_view f) {
            if (best_hints.hints.size() >= kMaxHints) return;
            uint8_t distance = strings_internal::CappedDamerauLevenshteinDistance(
                    flag, f, best_hints.best_distance);
            best_hints.AddHint(turbo::str_cat(f, " (undef_ok)"), distance);
        });
        return best_hints.hints;
    }

    // --------------------------------------------------------------------

    std::vector<char *> ParseCommandLineImpl(int argc, char *argv[],
                                             UsageFlagsAction usage_flag_action,
                                             OnUndefinedFlag undef_flag_action,
                                             std::ostream &error_help_output) {
        std::vector<char *> positional_args;
        std::vector<UnrecognizedFlag> unrecognized_flags;

        auto help_mode = flags_internal::parse_turbo_flags_only_impl(
                argc, argv, positional_args, unrecognized_flags, usage_flag_action);

        if (undef_flag_action != OnUndefinedFlag::kIgnoreUndefined) {
            flags_internal::report_unrecognized_flags(
                    unrecognized_flags,
                    (undef_flag_action == OnUndefinedFlag::kAbortIfUndefined));

            if (undef_flag_action == OnUndefinedFlag::kAbortIfUndefined) {
                if (!unrecognized_flags.empty()) {
                    flags_internal::HandleUsageFlags(error_help_output,
                                                     program_usage_message());
                    std::exit(1);
                }
            }
        }

        flags_internal::MaybeExit(help_mode);

        return positional_args;
    }

    // --------------------------------------------------------------------

    // This function handles all Turbo Flags and built-in usage flags and, if any
    // help mode was handled, it returns that help mode. The caller of this function
    // can decide to exit based on the returned help mode.
    // The caller may decide to handle unrecognized positional arguments and
    // unrecognized flags first before exiting.
    //
    // Returns:
    // * HelpMode::kFull if parsing errors were detected in recognized arguments
    // * The HelpMode that was handled in case when `usage_flag_action` is
    //   UsageFlagsAction::kHandleUsage and a usage flag was specified on the
    //   commandline
    // * Otherwise it returns HelpMode::kNone
    HelpMode parse_turbo_flags_only_impl(
            int argc, char *argv[], std::vector<char *> &positional_args,
            std::vector<UnrecognizedFlag> &unrecognized_flags,
            UsageFlagsAction usage_flag_action) {
        TURBO_INTERNAL_CHECK(argc > 0, "Missing argv[0]");

        using flags_internal::ArgsList;
        using flags_internal::specified_flags;

        std::vector<std::string> flagfile_value;
        std::vector<ArgsList> input_args;

        // Once parsing has started we will not allow more flag registrations.
        flags_internal::FinalizeRegistry();

        // This routine does not return anything since we abort on failure.
        flags_internal::CheckDefaultValuesParsingRoundtrip();

        input_args.push_back(ArgsList(argc, argv));

        // Set program invocation name if it is not set before.
        if (flags_internal::ProgramInvocationName() == "UNKNOWN") {
            flags_internal::SetProgramInvocationName(argv[0]);
        }
        positional_args.push_back(argv[0]);

        turbo::MutexLock l(&flags_internal::specified_flags_guard);
        if (specified_flags == nullptr) {
            specified_flags = new std::vector<const CommandLineFlag *>;
        } else {
            specified_flags->clear();
        }

        // Iterate through the list of the input arguments. First level are
        // arguments originated from argc/argv. Following levels are arguments
        // originated from recursive parsing of flags_file(s).
        bool success = true;
        while (!input_args.empty()) {
            // First we process the built-in generator flags.
            success &= flags_internal::HandleGeneratorFlags(input_args, flagfile_value);

            // Select top-most (most recent) arguments list. If it is empty drop it
            // and re-try.
            ArgsList &curr_list = input_args.back();

            // Every ArgsList starts with real or fake program name, so we can always
            // start by skipping it.
            curr_list.PopFront();

            if (curr_list.Size() == 0) {
                input_args.pop_back();
                continue;
            }

            // Handle the next argument in the current list. If the stack of argument
            // lists contains only one element - we are processing an argument from
            // the original argv.
            std::string_view arg(curr_list.Front());
            bool arg_from_argv = input_args.size() == 1;

            // If argument does not start with '-' or is just "-" - this is
            // positional argument.
            if (!turbo::consume_prefix(&arg, "-") || arg.empty()) {
                TURBO_INTERNAL_CHECK(arg_from_argv,
                                     "Flagfile cannot contain positional argument");

                positional_args.push_back(argv[curr_list.FrontIndex()]);
                continue;
            }

            // Split the current argument on '=' to deduce the argument flag name and
            // value. If flag name is empty it means we've got an "--" argument. Value
            // can be empty either if there were no '=' in argument string at all or
            // an argument looked like "--foo=". In a latter case is_empty_value is
            // true.
            std::string_view flag_name;
            std::string_view value;
            bool is_empty_value = false;

            std::tie(flag_name, value, is_empty_value) =
                    flags_internal::SplitNameAndValue(arg);

            // Standalone "--" argument indicates that the rest of the arguments are
            // positional. We do not support positional arguments in flagfiles.
            if (flag_name.empty()) {
                TURBO_INTERNAL_CHECK(arg_from_argv,
                                     "Flagfile cannot contain positional argument");

                curr_list.PopFront();
                break;
            }

            // Locate the flag based on flag name. Handle both --foo and --nofoo.
            CommandLineFlag *flag = nullptr;
            bool is_negative = false;
            std::tie(flag, is_negative) = flags_internal::LocateFlag(flag_name);

            if (flag == nullptr) {
                // Usage flags are not modeled as Turbo flags. Locate them separately.
                if (flags_internal::DeduceUsageFlags(flag_name, value)) {
                    continue;
                }
                unrecognized_flags.emplace_back(arg_from_argv
                                                ? UnrecognizedFlag::kFromArgv
                                                : UnrecognizedFlag::kFromFlagfile,
                                                flag_name);
                continue;
            }

            // Deduce flag's value (from this or next argument).
            bool value_success = true;
            std::tie(value_success, value) = flags_internal::DeduceFlagValue(
                    *flag, value, is_negative, is_empty_value, &curr_list);
            success &= value_success;

            // Set the located flag to a new value, unless it is retired. Setting
            // retired flag fails, but we ignoring it here while also reporting access
            // to retired flag.
            std::string error;
            if (!flags_internal::PrivateHandleAccessor::parse_from(
                    *flag, value, flags_internal::SET_FLAGS_VALUE,
                    flags_internal::kCommandLine, error)) {
                if (flag->is_retired()) continue;

                flags_internal::ReportUsageError(error, true);
                success = false;
            } else {
                specified_flags->push_back(flag);
            }
        }

        flags_internal::ResetGeneratorFlags(flagfile_value);

        // All the remaining arguments are positional.
        if (!input_args.empty()) {
            for (size_t arg_index = input_args.back().FrontIndex();
                 arg_index < static_cast<size_t>(argc); ++arg_index) {
                positional_args.push_back(argv[arg_index]);
            }
        }

        // Trim and sort the vector.
        specified_flags->shrink_to_fit();
        std::sort(specified_flags->begin(), specified_flags->end(),
                  flags_internal::SpecifiedFlagsCompare{});

        // Filter out unrecognized flags, which are ok to ignore.
        std::vector<UnrecognizedFlag> filtered;
        filtered.reserve(unrecognized_flags.size());
        for (const auto &unrecognized: unrecognized_flags) {
            if (flags_internal::CanIgnoreUndefinedFlag(unrecognized.flag_name))
                continue;
            filtered.push_back(unrecognized);
        }

        std::swap(unrecognized_flags, filtered);

        if (!success) {
#if TURBO_FLAGS_STRIP_NAMES
            flags_internal::ReportUsageError(
                "NOTE: command line flags are disabled in this build", true);
#else
            flags_internal::HandleUsageFlags(std::cerr, program_usage_message());
#endif
            return HelpMode::kFull;  // We just need to make sure the exit with
            // code 1.
        }

        return usage_flag_action == UsageFlagsAction::kHandleUsage
               ? flags_internal::HandleUsageFlags(std::cout,
                                                  program_usage_message())
               : HelpMode::kNone;
    }

}  // namespace turbo::flags_internal

namespace turbo {
    void parse_turbo_flags_only(int argc, char *argv[],
                                std::vector<char *> &positional_args,
                                std::vector<UnrecognizedFlag> &unrecognized_flags) {
        auto help_mode = flags_internal::parse_turbo_flags_only_impl(
                argc, argv, positional_args, unrecognized_flags,
                flags_internal::UsageFlagsAction::kHandleUsage);

        flags_internal::MaybeExit(help_mode);
    }

    // --------------------------------------------------------------------

    void report_unrecognized_flags(
            const std::vector<UnrecognizedFlag> &unrecognized_flags) {
        flags_internal::report_unrecognized_flags(unrecognized_flags, true);
    }

    // --------------------------------------------------------------------

    std::vector<char *> parse_command_line(int argc, char *argv[]) {
        return flags_internal::ParseCommandLineImpl(
                argc, argv, flags_internal::UsageFlagsAction::kHandleUsage,
                flags_internal::OnUndefinedFlag::kAbortIfUndefined);
    }

}  // namespace turbo
