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

#include <turbo/log/internal/vlog_config.h>

#include <stddef.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/const_init.h>
#include <turbo/base/internal/spinlock.h>
#include <turbo/base/no_destructor.h>
#include <turbo/base/optimization.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/log/internal/fnmatch.h>
#include <turbo/memory/memory.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/str_split.h>
#include <turbo/strings/string_view.h>
#include <turbo/strings/strip.h>
#include <turbo/synchronization/mutex.h>
#include <optional>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

namespace {
bool ModuleIsPath(turbo::string_view module_pattern) {
#ifdef _WIN32
  return module_pattern.find_first_of("/\\") != module_pattern.npos;
#else
  return module_pattern.find('/') != module_pattern.npos;
#endif
}
}  // namespace

bool VLogSite::SlowIsEnabled(int stale_v, int level) {
  if (TURBO_LIKELY(stale_v != kUninitialized)) {
    // Because of the prerequisites to this function, we know that stale_v is
    // either uninitialized or >= level. If it's not uninitialized, that means
    // it must be >= level, thus we should log.
    return true;
  }
  stale_v = log_internal::RegisterAndInitialize(this);
  return TURBO_UNLIKELY(stale_v >= level);
}

bool VLogSite::SlowIsEnabled0(int stale_v) { return SlowIsEnabled(stale_v, 0); }
bool VLogSite::SlowIsEnabled1(int stale_v) { return SlowIsEnabled(stale_v, 1); }
bool VLogSite::SlowIsEnabled2(int stale_v) { return SlowIsEnabled(stale_v, 2); }
bool VLogSite::SlowIsEnabled3(int stale_v) { return SlowIsEnabled(stale_v, 3); }
bool VLogSite::SlowIsEnabled4(int stale_v) { return SlowIsEnabled(stale_v, 4); }
bool VLogSite::SlowIsEnabled5(int stale_v) { return SlowIsEnabled(stale_v, 5); }

namespace {
struct VModuleInfo final {
  std::string module_pattern;
  bool module_is_path;  // i.e. it contains a path separator.
  int vlog_level;

  // Allocates memory.
  VModuleInfo(turbo::string_view module_pattern_arg, bool module_is_path_arg,
              int vlog_level_arg)
      : module_pattern(std::string(module_pattern_arg)),
        module_is_path(module_is_path_arg),
        vlog_level(vlog_level_arg) {}
};

// `mutex` guards all of the data structures that aren't lock-free.
// To avoid problems with the heap checker which calls into `VLOG`, `mutex` must
// be a `SpinLock` that prevents fiber scheduling instead of a `Mutex`.
TURBO_CONST_INIT turbo::base_internal::SpinLock mutex(
    turbo::kConstInit, turbo::base_internal::SCHEDULE_KERNEL_ONLY);

// `GetUpdateSitesMutex()` serializes updates to all of the sites (i.e. those in
// `site_list_head`) themselves.
turbo::Mutex* GetUpdateSitesMutex() {
  // Chromium requires no global destructors, so we can't use the
  // turbo::kConstInit idiom since turbo::Mutex as a non-trivial destructor.
  static turbo::NoDestructor<turbo::Mutex> update_sites_mutex TURBO_ACQUIRED_AFTER(
      mutex);
  return update_sites_mutex.get();
}

TURBO_CONST_INIT int global_v TURBO_GUARDED_BY(mutex) = 0;
// `site_list_head` is the head of a singly-linked list.  Traversal, insertion,
// and reads are atomic, so no locks are required, but updates to existing
// elements are guarded by `GetUpdateSitesMutex()`.
TURBO_CONST_INIT std::atomic<VLogSite*> site_list_head{nullptr};
TURBO_CONST_INIT std::vector<VModuleInfo>* vmodule_info TURBO_GUARDED_BY(mutex)
    TURBO_PT_GUARDED_BY(mutex){nullptr};

// Only used for lisp.
TURBO_CONST_INIT std::vector<std::function<void()>>* update_callbacks
    TURBO_GUARDED_BY(GetUpdateSitesMutex())
        TURBO_PT_GUARDED_BY(GetUpdateSitesMutex()){nullptr};

// Allocates memory.
std::vector<VModuleInfo>& get_vmodule_info()
    TURBO_EXCLUSIVE_LOCKS_REQUIRED(mutex) {
  if (!vmodule_info) vmodule_info = new std::vector<VModuleInfo>;
  return *vmodule_info;
}

// Does not allocate or take locks.
int VLogLevel(turbo::string_view file, const std::vector<VModuleInfo>* infos,
              int current_global_v) {
  // `infos` is null during a call to `VLOG` prior to setting `vmodule` (e.g. by
  // parsing flags).  We can't allocate in `VLOG`, so we treat null as empty
  // here and press on.
  if (!infos || infos->empty()) return current_global_v;
  // Get basename for file
  turbo::string_view basename = file;
  {
    const size_t sep = basename.rfind('/');
    if (sep != basename.npos) {
      basename.remove_prefix(sep + 1);
#ifdef _WIN32
    } else {
      const size_t sep = basename.rfind('\\');
      if (sep != basename.npos) basename.remove_prefix(sep + 1);
#endif
    }
  }

  turbo::string_view stem = file, stem_basename = basename;
  {
    const size_t sep = stem_basename.find('.');
    if (sep != stem_basename.npos) {
      stem.remove_suffix(stem_basename.size() - sep);
      stem_basename.remove_suffix(stem_basename.size() - sep);
    }
    if (turbo::consume_suffix(&stem_basename, "-inl")) {
      stem.remove_suffix(turbo::string_view("-inl").size());
    }
  }
  for (const auto& info : *infos) {
    if (info.module_is_path) {
      // If there are any slashes in the pattern, try to match the full
      // name.
      if (FNMatch(info.module_pattern, stem)) {
        return info.vlog_level == kUseFlag ? current_global_v : info.vlog_level;
      }
    } else if (FNMatch(info.module_pattern, stem_basename)) {
      return info.vlog_level == kUseFlag ? current_global_v : info.vlog_level;
    }
  }

  return current_global_v;
}

// Allocates memory.
int AppendVModuleLocked(turbo::string_view module_pattern, int log_level)
    TURBO_EXCLUSIVE_LOCKS_REQUIRED(mutex) {
  for (const auto& info : get_vmodule_info()) {
    if (FNMatch(info.module_pattern, module_pattern)) {
      // This is a memory optimization to avoid storing patterns that will never
      // match due to exit early semantics. Primarily optimized for our own unit
      // tests.
      return info.vlog_level;
    }
  }
  bool module_is_path = ModuleIsPath(module_pattern);
  get_vmodule_info().emplace_back(std::string(module_pattern), module_is_path,
                                  log_level);
  return global_v;
}

// Allocates memory.
int PrependVModuleLocked(turbo::string_view module_pattern, int log_level)
    TURBO_EXCLUSIVE_LOCKS_REQUIRED(mutex) {
  std::optional<int> old_log_level;
  for (const auto& info : get_vmodule_info()) {
    if (FNMatch(info.module_pattern, module_pattern)) {
      old_log_level = info.vlog_level;
      break;
    }
  }
  bool module_is_path = ModuleIsPath(module_pattern);
  auto iter = get_vmodule_info().emplace(get_vmodule_info().cbegin(),
                                         std::string(module_pattern),
                                         module_is_path, log_level);

  // This is a memory optimization to avoid storing patterns that will never
  // match due to exit early semantics. Primarily optimized for our own unit
  // tests.
  get_vmodule_info().erase(
      std::remove_if(++iter, get_vmodule_info().end(),
                     [module_pattern](const VModuleInfo& info) {
                       return FNMatch(info.module_pattern, module_pattern);
                     }),
      get_vmodule_info().cend());
  return old_log_level.value_or(global_v);
}
}  // namespace

int VLogLevel(turbo::string_view file) TURBO_LOCKS_EXCLUDED(mutex) {
  turbo::base_internal::SpinLockHolder l(&mutex);
  return VLogLevel(file, vmodule_info, global_v);
}

int RegisterAndInitialize(VLogSite* v) TURBO_LOCKS_EXCLUDED(mutex) {
  // std::memory_order_seq_cst is overkill in this function, but given that this
  // path is intended to be slow, it's not worth the brain power to relax that.
  VLogSite* h = site_list_head.load(std::memory_order_seq_cst);

  VLogSite* old = nullptr;
  if (v->next_.compare_exchange_strong(old, h, std::memory_order_seq_cst,
                                       std::memory_order_seq_cst)) {
    // Multiple threads may attempt to register this site concurrently.
    // By successfully setting `v->next` this thread commits to being *the*
    // thread that installs `v` in the list.
    while (!site_list_head.compare_exchange_weak(
        h, v, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
      v->next_.store(h, std::memory_order_seq_cst);
    }
  }

  int old_v = VLogSite::kUninitialized;
  int new_v = VLogLevel(v->file_);
  // No loop, if someone else set this, we should respect their evaluation of
  // `VLogLevel`. This may mean we return a stale `v`, but `v` itself will
  // always arrive at the freshest value.  Otherwise, we could be writing a
  // stale value and clobbering the fresher one.
  if (v->v_.compare_exchange_strong(old_v, new_v, std::memory_order_seq_cst,
                                    std::memory_order_seq_cst)) {
    return new_v;
  }
  return old_v;
}

void UpdateVLogSites() TURBO_UNLOCK_FUNCTION(mutex)
    TURBO_LOCKS_EXCLUDED(GetUpdateSitesMutex()) {
  std::vector<VModuleInfo> infos = get_vmodule_info();
  int current_global_v = global_v;
  // We need to grab `GetUpdateSitesMutex()` before we release `mutex` to ensure
  // that updates are not interleaved (resulting in an inconsistent final state)
  // and to ensure that the final state in the sites matches the final state of
  // `vmodule_info`. We unlock `mutex` to ensure that uninitialized sites don't
  // have to wait on all updates in order to acquire `mutex` and initialize
  // themselves.
  turbo::MutexLock ul(GetUpdateSitesMutex());
  mutex.Unlock();
  VLogSite* n = site_list_head.load(std::memory_order_seq_cst);
  // Because sites are added to the list in the order they are executed, there
  // tend to be clusters of entries with the same file.
  const char* last_file = nullptr;
  int last_file_level = 0;
  while (n != nullptr) {
    if (n->file_ != last_file) {
      last_file = n->file_;
      last_file_level = VLogLevel(n->file_, &infos, current_global_v);
    }
    n->v_.store(last_file_level, std::memory_order_seq_cst);
    n = n->next_.load(std::memory_order_seq_cst);
  }
  if (update_callbacks) {
    for (auto& cb : *update_callbacks) {
      cb();
    }
  }
}

void UpdateVModule(turbo::string_view vmodule)
    TURBO_LOCKS_EXCLUDED(mutex, GetUpdateSitesMutex()) {
  std::vector<std::pair<turbo::string_view, int>> glob_levels;
  for (turbo::string_view glob_level : turbo::str_split(vmodule, ',')) {
    const size_t eq = glob_level.rfind('=');
    if (eq == glob_level.npos) continue;
    const turbo::string_view glob = glob_level.substr(0, eq);
    int level;
    if (!turbo::simple_atoi(glob_level.substr(eq + 1), &level)) continue;
    glob_levels.emplace_back(glob, level);
  }
  mutex.Lock();  // Unlocked by UpdateVLogSites().
  get_vmodule_info().clear();
  for (const auto& it : glob_levels) {
    const turbo::string_view glob = it.first;
    const int level = it.second;
    AppendVModuleLocked(glob, level);
  }
  UpdateVLogSites();
}

int UpdateGlobalVLogLevel(int v)
    TURBO_LOCKS_EXCLUDED(mutex, GetUpdateSitesMutex()) {
  mutex.Lock();  // Unlocked by UpdateVLogSites().
  const int old_global_v = global_v;
  if (v == global_v) {
    mutex.Unlock();
    return old_global_v;
  }
  global_v = v;
  UpdateVLogSites();
  return old_global_v;
}

int PrependVModule(turbo::string_view module_pattern, int log_level)
    TURBO_LOCKS_EXCLUDED(mutex, GetUpdateSitesMutex()) {
  mutex.Lock();  // Unlocked by UpdateVLogSites().
  int old_v = PrependVModuleLocked(module_pattern, log_level);
  UpdateVLogSites();
  return old_v;
}

void OnVLogVerbosityUpdate(std::function<void()> cb)
    TURBO_LOCKS_EXCLUDED(GetUpdateSitesMutex()) {
  turbo::MutexLock ul(GetUpdateSitesMutex());
  if (!update_callbacks)
    update_callbacks = new std::vector<std::function<void()>>;
  update_callbacks->push_back(std::move(cb));
}

VLogSite* SetVModuleListHeadForTestOnly(VLogSite* v) {
  return site_list_head.exchange(v, std::memory_order_seq_cst);
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo
