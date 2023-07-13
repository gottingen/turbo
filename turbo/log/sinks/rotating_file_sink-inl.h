// Copyright 2023 The titan-search Authors.
// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
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

#pragma once

#include <turbo/log/sinks/rotating_file_sink.h>

#include "turbo/log/common.h"
#include "turbo/files/utility.h"
#include "turbo/files/sequential_write_file.h"
#include "turbo/log/details/null_mutex.h"
#include <turbo/format/format.h>

#include <cerrno>
#include <chrono>
#include <ctime>
#include <mutex>
#include <string>
#include <tuple>

namespace turbo::tlog {
    namespace sinks {

        template<typename Mutex>
        rotating_file_sink<Mutex>::rotating_file_sink(
                filename_t base_filename, std::size_t max_size, std::size_t max_files, bool rotate_on_open,
                const turbo::FileEventListener &event_handlers)
                : base_filename_(std::move(base_filename)), max_size_(max_size), max_files_(max_files),
                  file_writer_{event_handlers} {
            if (max_size == 0) {
                throw_tlog_ex("rotating sink constructor: max_size arg cannot be zero");
            }

            if (max_files > 200000) {
                throw_tlog_ex("rotating sink constructor: max_files arg cannot exceed 200000");
            }
            auto r = file_writer_.open(calc_filename(base_filename_, 0));
            if (!r.ok()) {
                throw_tlog_ex(r.ToString());
            }
            auto rs = file_writer_.size();
            if (!rs.ok()) {
                throw_tlog_ex(r.ToString());
            }
            current_size_ = rs.value(); // expensive. called only once
            if (rotate_on_open && current_size_ > 0) {
                rotate_();
                current_size_ = 0;
            }
        }

// calc filename according to index and file extension if exists.
// e.g. calc_filename("logs/mylog.txt, 3) => "logs/mylog.3.txt".
        template<typename Mutex>
        filename_t rotating_file_sink<Mutex>::calc_filename(const filename_t &filename, std::size_t index) {
            if (index == 0u) {
                return filename;
            }

            filename_t basename, ext;
            std::tie(basename, ext) = turbo::FileUtility::split_by_extension(filename);
            return fmt_lib::format(TLOG_FILENAME_T("{}.{}{}"), basename, index, ext);
        }

        template<typename Mutex>
        filename_t rotating_file_sink<Mutex>::filename() {
            std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
            return file_writer_.file_path().native();
        }

        template<typename Mutex>
        void rotating_file_sink<Mutex>::sink_it_(const details::log_msg &msg) {
            memory_buf_t formatted;
            base_sink<Mutex>::formatter_->format(msg, formatted);
            auto new_size = current_size_ + formatted.size();

            // rotate if the new estimated file size exceeds max size.
            // rotate only if the real size > 0 to better deal with full disk (see issue #2261).
            // we only check the real size when new_size > max_size_ because it is relatively expensive.
            if (new_size > max_size_) {
                auto r = file_writer_.flush();
                if(!r.ok()) {
                    throw_tlog_ex(r.ToString());
                }

                if (file_writer_.size().value_or(0) > 0) {
                    rotate_();
                    new_size = formatted.size();
                }
            }
            auto r = file_writer_.write(formatted);
            if(!r.ok()) {
                throw_tlog_ex(r.ToString());
            }
            current_size_ = new_size;
        }

        template<typename Mutex>
        void rotating_file_sink<Mutex>::flush_() {
            auto r = file_writer_.flush();
            if(!r.ok()) {
                throw_tlog_ex(r.ToString());
            }
        }

        // Rotate files:
        // log.txt -> log.1.txt
        // log.1.txt -> log.2.txt
        // log.2.txt -> log.3.txt
        // log.3.txt -> delete
        template<typename Mutex>
        void rotating_file_sink<Mutex>::rotate_() {
            using details::os::filename_to_str;
            using details::os::path_exists;

            file_writer_.close();
            for (auto i = max_files_; i > 0; --i) {
                filename_t src = calc_filename(base_filename_, i - 1);
                if (!path_exists(src)) {
                    continue;
                }
                filename_t target = calc_filename(base_filename_, i);

                if (!rename_file_(src, target)) {
                    // if failed try again after a small delay.
                    // this is a workaround to a windows issue, where very high rotation
                    // rates can cause the rename to fail with permission denied (because of antivirus?).
                    details::os::sleep_for_millis(100);
                    if (!rename_file_(src, target)) {
                        auto r = file_writer_.reopen(
                                true); // truncate the log file anyway to prevent it to grow beyond its limit!
                        TURBO_UNUSED(r);
                        current_size_ = 0;
                        throw_tlog_ex("rotating_file_sink: failed renaming " + filename_to_str(src) + " to " +
                                      filename_to_str(target), errno);
                    }
                }
            }
            auto r = file_writer_.reopen(true);
            if(!r.ok()) {
                throw_tlog_ex(r.ToString());
            }
        }

        // delete the target if exists, and rename the src file  to target
        // return true on success, false otherwise.
        template<typename Mutex>
        bool
        rotating_file_sink<Mutex>::rename_file_(const filename_t &src_filename, const filename_t &target_filename) {
            // try to delete the target file in case it already exists.
            (void) details::os::remove(target_filename);
            return details::os::rename(src_filename, target_filename) == 0;
        }

    } // namespace sinks
} // namespace turbo::tlog
