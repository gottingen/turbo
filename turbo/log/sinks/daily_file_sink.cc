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

#include "turbo/log/sinks/daily_file_sink.h"
#include "turbo/time/civil_time.h"
#include "turbo/container/flat_hash_set.h"

namespace turbo {

DailyFileSink::DailyFileSink(const turbo::filesystem::path &file_name,
                             int rotation_hour, int rotation_minute,
                             bool truncate, uint16_t max_files)
    : base_filename_(file_name),
      rotation_h_(rotation_hour),
      rotation_m_(rotation_minute),
      truncate_(truncate),
      max_files_(max_files){

}

turbo::filesystem::path DailyFileSink::calc_filename(const turbo::Time &t) {
  turbo::TimeZone tz;
  auto civil_day = turbo::ToCivilDay(t, tz);
  return turbo::StrFormat("%s_%4d-%02d-%02d.%s",
                        base_filename_.filename(),
                       civil_day.year(),
                       civil_day.month(),
                       civil_day.day(),
                       base_filename_.extension());
}

turbo::Status DailyFileSink::init() {
  if (rotation_h_ < 0 || rotation_h_ > 23 || rotation_m_ < 0 || rotation_m_ > 59) {
    return turbo::Status(turbo::kInvalidArgument, "daily_file_sink: Invalid rotation time in ctor");
  }
  init_file_queue();
  auto now = turbo::Now();
  auto filename = calc_filename(now);
  log_file_.reset(new turbo::SequentialWriteFile());
  auto rs = log_file_->open(filename, truncate_);
  if(!rs.ok()) {
    return rs;
  }
  turbo::TimeZone tz;
  auto next_day = now + turbo::Duration(turbo::Hours(24));
  auto civil_now = turbo::ToCivilDay(next_day, tz);
  CivilSecond next_tp(civil_now.year(),civil_now.month(),civil_now.day(),rotation_h_, rotation_m_,0);
  rotation_tp_ = next_rotate_time();
  return OkStatus();
}

turbo::Time DailyFileSink::next_rotate_time() {
  turbo::TimeZone tz;
  auto now = turbo::Now();
  auto next_day = now + turbo::Duration(turbo::Hours(24));
  auto civil_now = turbo::ToCivilDay(next_day, tz);
  CivilSecond next_tp(civil_now.year(),civil_now.month(),civil_now.day(),rotation_h_, rotation_m_,0);
  return turbo::FromCivil(next_tp, tz);
}

void DailyFileSink::init_file_queue() {
  filenames_q_.resize(max_files_);
  turbo::flat_hash_set<turbo::filesystem::path> reserved;
  turbo::Time now = turbo::Now();
  std::error_code ec;
  size_t cnt = 0;
  while (cnt < max_files_) {
    auto fname = calc_filename(now);
    if(turbo::filesystem::exists(fname)) {
      filenames_q_.push_back(fname);
      reserved.insert(fname);
    }
    now -= turbo::Hours(24);
    ++cnt;
  }

  auto dir_name = base_filename_.parent_path();
  turbo::filesystem::directory_iterator it(dir_name);
  turbo::filesystem::directory_iterator it_end;
  while(it != it_end) {
    if(it->path() == ".." || it->path() == ".") {
      continue ;
    }
    if(reserved.find(it->path()) == reserved.end()) {
      turbo::filesystem::remove(it->path());
    }
  }

}
void DailyFileSink::remove_old_files() {
  auto current_file = log_file_->file_path();
  turbo::filesystem::path old_file;
  std::error_code ec;
  if(max_files_ == filenames_q_.size()) {
    old_file = filenames_q_.front();
    filenames_q_.pop_front();
  }
  if(!old_file.empty() && turbo::filesystem::exists(old_file, ec)) {
    turbo::filesystem::remove(old_file, ec);
  }
  filenames_q_.push_back(current_file);
}
void DailyFileSink::rotate(const turbo::Time &t) {
  log_file_->flush();
  log_file_->close();
  remove_old_files();
  auto fname = calc_filename(t);
  log_file_.reset(new SequentialWriteFile());
  auto rs = log_file_->open(fname, truncate_);
  TURBO_UNUSED(rs);
}
void DailyFileSink::Send(const turbo::LogEntry &entry) {
  auto t = entry.timestamp();
  if(t >= rotation_tp_) {
    rotate(t);
  }
  string_piece text =entry.text_message_with_prefix_and_newline();
  auto rs = log_file_->append(text);
  TURBO_UNUSED(rs);
}
void DailyFileSink::Flush() {
  log_file_->flush();
}

} // namespace turbo