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
//

#include "turbo/debugging/failure_signal_handler.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "turbo/base/internal/raw_logging.h"
#include "turbo/debugging/stacktrace.h"
#include "turbo/debugging/symbolize.h"
#include "turbo/strings/match.h"
#include "turbo/strings/str_cat.h"

namespace {

using testing::StartsWith;

#if GTEST_HAS_DEATH_TEST

// For the parameterized death tests. GetParam() returns the signal number.
using FailureSignalHandlerDeathTest = ::testing::TestWithParam<int>;

// This function runs in a fork()ed process on most systems.
void InstallHandlerAndRaise(int signo) {
  turbo::InstallFailureSignalHandler(turbo::FailureSignalHandlerOptions());
  raise(signo);
}

TEST_P(FailureSignalHandlerDeathTest, TurboFailureSignal) {
  const int signo = GetParam();
  std::string exit_regex = turbo::StrCat(
      "\\*\\*\\* ", turbo::debugging_internal::FailureSignalToString(signo),
      " received at time=");
#ifndef _WIN32
  EXPECT_EXIT(InstallHandlerAndRaise(signo), testing::KilledBySignal(signo),
              exit_regex);
#else
  // Windows doesn't have testing::KilledBySignal().
  EXPECT_DEATH_IF_SUPPORTED(InstallHandlerAndRaise(signo), exit_regex);
#endif
}

TURBO_CONST_INIT FILE* error_file = nullptr;

void WriteToErrorFile(const char* msg) {
  if (msg != nullptr) {
    TURBO_RAW_CHECK(fwrite(msg, strlen(msg), 1, error_file) == 1,
                   "fwrite() failed");
  }
  TURBO_RAW_CHECK(fflush(error_file) == 0, "fflush() failed");
}

std::string GetTmpDir() {
  // TEST_TMPDIR is set by Bazel. Try the others when not running under Bazel.
  static const char* const kTmpEnvVars[] = {"TEST_TMPDIR", "TMPDIR", "TEMP",
                                            "TEMPDIR", "TMP"};
  for (const char* const var : kTmpEnvVars) {
    const char* tmp_dir = std::getenv(var);
    if (tmp_dir != nullptr) {
      return tmp_dir;
    }
  }

  // Try something reasonable.
  return "/tmp";
}

// This function runs in a fork()ed process on most systems.
void InstallHandlerWithWriteToFileAndRaise(const char* file, int signo) {
  error_file = fopen(file, "w");
  TURBO_RAW_CHECK(error_file != nullptr, "Failed create error_file");
  turbo::FailureSignalHandlerOptions options;
  options.writerfn = WriteToErrorFile;
  turbo::InstallFailureSignalHandler(options);
  raise(signo);
}

TEST_P(FailureSignalHandlerDeathTest, TurboFatalSignalsWithWriterFn) {
  const int signo = GetParam();
  std::string tmp_dir = GetTmpDir();
  std::string file = turbo::StrCat(tmp_dir, "/signo_", signo);

  std::string exit_regex = turbo::StrCat(
      "\\*\\*\\* ", turbo::debugging_internal::FailureSignalToString(signo),
      " received at time=");
#ifndef _WIN32
  EXPECT_EXIT(InstallHandlerWithWriteToFileAndRaise(file.c_str(), signo),
              testing::KilledBySignal(signo), exit_regex);
#else
  // Windows doesn't have testing::KilledBySignal().
  EXPECT_DEATH_IF_SUPPORTED(
      InstallHandlerWithWriteToFileAndRaise(file.c_str(), signo), exit_regex);
#endif

  // Open the file in this process and check its contents.
  std::fstream error_output(file);
  ASSERT_TRUE(error_output.is_open()) << file;
  std::string error_line;
  std::getline(error_output, error_line);
  EXPECT_THAT(
      error_line,
      StartsWith(turbo::StrCat(
          "*** ", turbo::debugging_internal::FailureSignalToString(signo),
          " received at ")));

  // On platforms where it is possible to get the current CPU, the
  // CPU number is also logged. Check that it is present in output.
#if defined(__linux__)
  EXPECT_THAT(error_line, testing::HasSubstr(" on cpu "));
#endif

  if (turbo::debugging_internal::StackTraceWorksForTest()) {
    std::getline(error_output, error_line);
    EXPECT_THAT(error_line, StartsWith("PC: "));
  }
}

constexpr int kFailureSignals[] = {
    SIGSEGV, SIGILL,  SIGFPE, SIGABRT, SIGTERM,
#ifndef _WIN32
    SIGBUS,  SIGTRAP,
#endif
};

std::string SignalParamToString(const ::testing::TestParamInfo<int>& info) {
  std::string result =
      turbo::debugging_internal::FailureSignalToString(info.param);
  if (result.empty()) {
    result = turbo::StrCat(info.param);
  }
  return result;
}

INSTANTIATE_TEST_SUITE_P(TurboDeathTest, FailureSignalHandlerDeathTest,
                         ::testing::ValuesIn(kFailureSignals),
                         SignalParamToString);

#endif  // GTEST_HAS_DEATH_TEST

}  // namespace

int main(int argc, char** argv) {
  turbo::InitializeSymbolizer(argv[0]);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
