#
# Copyright 2023 The titan-search Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


set(TURBO_ALL_SRC
        "base/internal/raw_logging.cc"
        "base/internal/strerror.cc"
        "base/log_severity.cc"
        "base/status.cc"
        "base/result_status.cc"
        "base/int128.cc"
        "base/status_payload_printer.cc"
        "base/turbo_error.cc"
        "base/turbo_module.cc"
        "container/internal/hashtablez_sampler.cc"
        "container/internal/hashtablez_sampler_force_weak_definition.cc"
        "container/internal/raw_hash_set.cc"
        "concurrent/barrier.cc"
        "concurrent/hash_lock.cc"
        "concurrent/latch.cc"
        "concurrent/notification.cc"
        "crypto/crc32c.cc"
        "crypto/md5.cc"
        "crypto/sha1.cc"
        "crypto/sha256.cc"
        "crypto/sha512.cc"
        "crypto/internal/cpu_detect.cc"
        "crypto/internal/crc.cc"
        "crypto/internal/crc_cord_state.cc"
        "crypto/internal/crc_x86_arm_combined.cc"
        "crypto/internal/crc_memcpy_fallback.cc"
        "crypto/internal/crc_memcpy_x86_64.cc"
        "crypto/internal/crc_non_temporal_memcpy.cc"
        "crypto/internal/crc_x86_arm_combined.cc"
        "debugging/failure_signal_handler.cc"
        "debugging/stacktrace.cc"
        "debugging/symbolize.cc"
        "debugging/internal/address_is_readable.cc"
        "debugging/internal/demangle.cc"
        "debugging/internal/elf_mem_image.cc"
        "debugging/internal/examine_stack.cc"
        "../tests/debugging/stack_consumption.cc"
        "debugging/internal/vdso_support.cc"
        "debugging/leak_check.cc"
        "files/file_watcher.cc"
        "files/fio.cc"
        "files/sequential_read_file.cc"
        "files/sequential_write_file.cc"
        "flags/app.cc"
        "flags/config.cc"
        "flags/formatter.cc"
        "flags/option.cc"
        "flags/split.cc"
        "flags/string_tools.cc"
        "flags/validators.cc"
        "format/fmt/format.cc"
        "format/fmt/os.cc"
        "hash/internal/city.cc"
        "hash/internal/hash.cc"
        "hash/internal/low_level_hash.cc"
        "log/async.cc"
        "log/cfg.cc"
        "log/condition.cc"
        "log/color_sinks.cc"
        "log/file_sinks.cc"
        "log/logging.cc"
        "log/tlog.cc"
        "log/stdout_sinks.cc"
        "memory/jemalloc_helper.cc"
        "platform/internal/cycleclock.cc"
        "platform/internal/spinlock.cc"
        "platform/internal/spinlock_wait.cc"
        "platform/internal/sysinfo.cc"
        "platform/internal/scoped_set_env.cc"
        "base/internal/throw_delegate.cc"
        "platform/internal/unscaledcycleclock.cc"
        "platform/internal/low_level_alloc.cc"
        "profiling/internal/exponential_biased.cc"
        "profiling/internal/periodic_sampler.cc"
        "random/discrete_distribution.cc"
        "random/gaussian_distribution.cc"
        "random/internal/pool_urbg.cc"
        "random/internal/randen.cc"
        "random/internal/randen_detect.cc"
        "random/internal/randen_hwaes.cc"
        "random/internal/randen_round_keys.cc"
        "random/internal/randen_slow.cc"
        "random/internal/seed_material.cc"
        "random/seed_gen_exception.cc"
        "random/seed_sequences.cc"
        "strings/ascii.cc"
        "strings/charconv.cc"
        "strings/cord.cc"
        "strings/cord_analysis.cc"
        "strings/cord_buffer.cc"
        "strings/escaping.cc"
        "strings/match.cc"
        "strings/numbers.cc"
        "strings/str_case_conv.cc"
        "strings/str_replace.cc"
        "strings/str_split.cc"
        "strings/str_trim.cc"
        "strings/internal/charconv_bigint.cc"
        "strings/internal/charconv_parse.cc"
        "strings/internal/cord_internal.cc"
        "strings/internal/cord_rep_btree.cc"
        "strings/internal/cord_rep_btree_navigator.cc"
        "strings/internal/cord_rep_btree_reader.cc"
        "strings/internal/cord_rep_crc.cc"
        "strings/internal/cord_rep_consume.cc"
        "strings/internal/cord_rep_ring.cc"
        "strings/internal/cordz_functions.cc"
        "strings/internal/cordz_handle.cc"
        "strings/internal/cordz_info.cc"
        "strings/internal/cordz_sample_token.cc"
        "strings/internal/escaping.cc"
        "strings/internal/memutil.cc"
        "strings/internal/ostringstream.cc"
        "times/civil_time.cc"
        "times/clock.cc"
        "times/duration.cc"
        "times/format.cc"
        "times/stop_watcher.cc"
        "times/time.cc"
        "times/cctz/civil_time_detail.cc"
        "times/cctz/time_zone_fixed.cc"
        "times/cctz/time_zone_format.cc"
        "times/cctz/time_zone_if.cc"
        "times/cctz/time_zone_impl.cc"
        "times/cctz/time_zone_info.cc"
        "times/cctz/time_zone_libc.cc"
        "times/cctz/time_zone_lookup.cc"
        "times/cctz/time_zone_posix.cc"
        "times/cctz/zone_info_source.cc"
        "unicode/utf.cc"
        )

if (BUILD_STATIC_LIBRARY)
    carbin_cc_library(
            NAMESPACE
            turbo
            NAME
            turbo
            SOURCES
            ${TURBO_ALL_SRC}
            COPTS
            ${CARBIN_CXX_OPTIONS}
            DEPS
            ${CARBIN_DEPS_LINK}
            PUBLIC
    )
endif (BUILD_STATIC_LIBRARY)

if (BUILD_SHARED_LIBRARY)
    carbin_cc_library(
            NAMESPACE
            turbo
            NAME
            turbo
            SOURCES
            ${TURBO_ALL_SRC}
            COPTS
            ${CARBIN_CXX_OPTIONS}
            DEPS
            ${CARBIN_DEPS_LINK}
            SHARED
            PUBLIC
    )
endif (BUILD_SHARED_LIBRARY)

