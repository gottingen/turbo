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

#ifndef TURBO_LOG_SINK_DAILY_FILE_SINK_H_
#define TURBO_LOG_SINK_DAILY_FILE_SINK_H_

#include "turbo/files/filesystem.h"
#include "turbo/log/log_sink.h"
#include "turbo/strings/string_view.h"
#include "turbo/files/sequential_write_file.h"
#include "turbo/container/ring_buffer.h"
#include <array>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>

namespace turbo {

class DailyFileSink : public LogSink {
public:
  DailyFileSink(const turbo::filesystem::path &file_name, int rotation_hour,
                int rotation_minute, bool truncate = false,
                uint16_t max_files = 0);
  ~DailyFileSink() override = default;

  turbo::Status init();
  void Send(const turbo::LogEntry &entry) override;

  void Flush() override;

private:
  TURBO_NON_COPYABLE(DailyFileSink);
  TURBO_NON_MOVEABLE(DailyFileSink);
private:
  turbo::filesystem::path calc_filename(const turbo::Time &now);
  turbo::Time next_rotate_time();
  void init_file_queue();
  void remove_old_files();
  void rotate(const turbo::Time &t);
private:
  turbo::filesystem::path base_filename_;
  int rotation_h_;
  int rotation_m_;
  bool truncate_;
  uint16_t max_files_;
  std::unique_ptr<turbo::SequentialWriteFile> log_file_;
  turbo::Time rotation_tp_;
  turbo::ring_buffer<turbo::filesystem::path> filenames_q_;
};

} // namespace turbo

#endif // TURBO_LOG_SINK_DAILY_FILE_SINK_H_
