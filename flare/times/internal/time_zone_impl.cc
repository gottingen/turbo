
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#include "flare/times/internal/time_zone_impl.h"
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include "flare/base/profile.h"
#include "flare/times/internal/time_zone_fixed.h"

namespace flare::times_internal {


    namespace {

// time_zone::Impls are linked into a map to support fast lookup by name.
        using TimeZoneImplByName =
        std::unordered_map<std::string, const time_zone::Impl *>;
        TimeZoneImplByName *time_zone_map = nullptr;

// Mutual exclusion for time_zone_map.
        std::mutex &TimeZoneMutex() {
            // This mutex is intentionally "leaked" to avoid the static deinitialization
            // order fiasco (std::mutex's destructor is not trivial on many platforms).
            static std::mutex *time_zone_mutex = new std::mutex;
            return *time_zone_mutex;
        }

    }  // namespace

    time_zone time_zone::Impl::UTC() { return time_zone(UTCImpl()); }

    bool time_zone::Impl::load_time_zone(const std::string &name, time_zone *tz) {
        const time_zone::Impl *const utc_impl = UTCImpl();

        // First check for UTC (which is never a key in time_zone_map).
        auto offset = seconds::zero();
        if (fixed_offset_from_name(name, &offset) && offset == seconds::zero()) {
            *tz = time_zone(utc_impl);
            return true;
        }

        // Then check, under a shared lock, whether the time zone has already
        // been loaded. This is the common path. TODO: Move to shared_mutex.
        {
            std::lock_guard<std::mutex> lock(TimeZoneMutex());
            if (time_zone_map != nullptr) {
                TimeZoneImplByName::const_iterator itr = time_zone_map->find(name);
                if (itr != time_zone_map->end()) {
                    *tz = time_zone(itr->second);
                    return itr->second != utc_impl;
                }
            }
        }

        // Now check again, under an exclusive lock.
        std::lock_guard<std::mutex> lock(TimeZoneMutex());
        if (time_zone_map == nullptr) time_zone_map = new TimeZoneImplByName;
        const Impl *&impl = (*time_zone_map)[name];
        if (impl == nullptr) {
            // The first thread in loads the new time zone.
            Impl *new_impl = new Impl(name);
            new_impl->zone_ = time_zone_if::load(new_impl->name_);
            if (new_impl->zone_ == nullptr) {
                delete new_impl;  // free the nascent Impl
                impl = utc_impl;  // and fallback to UTC
            } else {
                impl = new_impl;  // install new time zone
            }
        }
        *tz = time_zone(impl);
        return impl != utc_impl;
    }

    void time_zone::Impl::ClearTimeZoneMapTestOnly() {
        std::lock_guard<std::mutex> lock(TimeZoneMutex());
        if (time_zone_map != nullptr) {
            // Existing time_zone::Impl* entries are in the wild, so we can't delete
            // them. Instead, we move them to a private container, where they are
            // logically unreachable but not "leaked".  Future requests will result
            // in reloading the data.
            static auto *cleared = new std::deque<const time_zone::Impl *>;
            for (const auto &element : *time_zone_map) {
                cleared->push_back(element.second);
            }
            time_zone_map->clear();
        }
    }

    time_zone::Impl::Impl(const std::string &name) : name_(name) {}

    const time_zone::Impl *time_zone::Impl::UTCImpl() {
        static Impl *utc_impl = [] {
            Impl *impl = new Impl("UTC");
            impl->zone_ = time_zone_if::load(impl->name_);  // never fails
            return impl;
        }();
        return utc_impl;
    }

}  // namespace flare::times_internal
