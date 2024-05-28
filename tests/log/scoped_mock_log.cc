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

#include <tests/log/scoped_mock_log.h>

#include <atomic>
#include <string>

#include <gmock/gmock.h>
#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/log/log_entry.h>
#include <turbo/log/log_sink.h>
#include <turbo/log/log_sink_registry.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

ScopedMockLog::ScopedMockLog(MockLogDefault default_exp)
    : sink_(this), is_capturing_logs_(false), is_triggered_(false) {
  if (default_exp == MockLogDefault::kIgnoreUnexpected) {
    // Ignore all calls to Log we did not set expectations for.
    EXPECT_CALL(*this, Log).Times(::testing::AnyNumber());
  } else {
    // Disallow all calls to Log we did not set expectations for.
    EXPECT_CALL(*this, Log).Times(0);
  }
  // By default Send mock forwards to Log mock.
  EXPECT_CALL(*this, Send)
      .Times(::testing::AnyNumber())
      .WillRepeatedly([this](const turbo::LogEntry& entry) {
        is_triggered_.store(true, std::memory_order_relaxed);
        Log(entry.log_severity(), std::string(entry.source_filename()),
            std::string(entry.text_message()));
      });

  // By default We ignore all Flush calls.
  EXPECT_CALL(*this, Flush).Times(::testing::AnyNumber());
}

ScopedMockLog::~ScopedMockLog() {
  TURBO_RAW_CHECK(is_triggered_.load(std::memory_order_relaxed),
                 "Did you forget to call StartCapturingLogs()?");

  if (is_capturing_logs_) StopCapturingLogs();
}

void ScopedMockLog::StartCapturingLogs() {
  TURBO_RAW_CHECK(!is_capturing_logs_,
                 "StartCapturingLogs() can be called only when the "
                 "turbo::ScopedMockLog object is not capturing logs.");

  is_capturing_logs_ = true;
  is_triggered_.store(true, std::memory_order_relaxed);
  turbo::add_log_sink(&sink_);
}

void ScopedMockLog::StopCapturingLogs() {
  TURBO_RAW_CHECK(is_capturing_logs_,
                 "StopCapturingLogs() can be called only when the "
                 "turbo::ScopedMockLog object is capturing logs.");

  is_capturing_logs_ = false;
  turbo::remove_log_sink(&sink_);
}

turbo::LogSink& ScopedMockLog::UseAsLocalSink() {
  is_triggered_.store(true, std::memory_order_relaxed);
  return sink_;
}

TURBO_NAMESPACE_END
}  // namespace turbo
