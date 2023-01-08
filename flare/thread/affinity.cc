
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include <unordered_set>
#include <pthread.h>
#include <unistd.h>
#include <thread>
#include <algorithm>

#if defined(__APPLE__)

#include <mach/thread_act.h>

#elif defined(__FreeBSD__)
#include <pthread_np.h>
#endif


#include "flare/thread/affinity.h"
#include "flare/base/profile.h"


namespace flare {

    namespace {

        struct core_hasher {
            inline uint64_t operator()(const flare::core_node &core) const {
                return core.index;
            }
        };

    }  // anonymous namespace

    core_affinity::core_affinity() : cores() {}

    core_affinity::core_affinity(core_affinity &&other) : cores(std::move(other.cores)) {}

    core_affinity::core_affinity(std::initializer_list<core_node> list)
            : cores() {
        cores.reserve(list.size());
        for (auto core : list) {
            cores.push_back(core);
        }
    }

    core_affinity core_affinity::all() {
        core_affinity affinity;

#if defined(__linux__) && !defined(__ANDROID__)
        auto thread = pthread_self();
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        if (pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset) == 0) {
              int count = CPU_COUNT(&cpuset);
              for (int i = 0; i < count; i++) {
                    core_node core;
                    core.index = static_cast<int32_t>(i);
                    affinity.cores.emplace_back(std::move(core));
              }
        }
#elif defined(__FreeBSD__)
        auto thread = pthread_self();
        cpuset_t cpuset;
        CPU_ZERO(&cpuset);
        if (pthread_getaffinity_np(thread, sizeof(cpuset_t), &cpuset) == 0) {
              int count = CPU_COUNT(&cpuset);
              for (int i = 0; i < count; i++) {
                    core_node core;
                    core.index = static_cast<int32_t>(i);
                    affinity.cores.emplace_back(std::move(core));
              }
        }
#else
        static_assert(!supported,
                      "flare::thread::core_affinity::supported is true, but "
                      "thread::core_affinity::all() is not implemented for this platform");
#endif

        return affinity;
    }

    std::shared_ptr<core_affinity::affinity_policy> core_affinity::affinity_policy::any_of(
            core_affinity &&affinity) {
        struct dump_policy : public core_affinity::affinity_policy {
            core_affinity affinity;

            dump_policy(core_affinity &&affinity) : affinity(std::move(affinity)) {}

            core_affinity get(uint32_t threadId) const override {

                auto count = affinity.count();
                if (count == 0) {
                    return core_affinity(affinity);
                }
                auto group = affinity[threadId % affinity.count()].group;
                core_affinity out;
                out.cores.reserve(count);
                for (auto core : affinity.cores) {
                    if (core.group == group) {
                        out.cores.push_back(core);
                    }
                }
                return out;
            }
        };

        return std::make_shared<dump_policy>(std::move(affinity));
    }

    std::shared_ptr<core_affinity::affinity_policy> core_affinity::affinity_policy::one_of(
            core_affinity &&affinity) {
        struct dump_policy : public core_affinity::affinity_policy {
            core_affinity affinity;

            dump_policy(core_affinity &&affinity) : affinity(std::move(affinity)) {}

            core_affinity get(uint32_t threadId) const override {
                auto count = affinity.count();
                if (count == 0) {
                    return core_affinity(affinity);
                }
                return core_affinity({affinity[threadId % affinity.count()]});
            }
        };

        return std::make_shared<dump_policy>(std::move(affinity));
    }

    size_t core_affinity::count() const {
        return cores.size();
    }

    core_node core_affinity::operator[](size_t index) const {
        return cores[index];
    }

    core_affinity &core_affinity::add(const core_affinity &other) {
        std::unordered_set<core_node, core_hasher> set;
        for (auto core : cores) {
            set.emplace(core);
        }
        for (auto core : other.cores) {
            if (set.count(core) == 0) {
                cores.push_back(core);
            }
        }
        std::sort(cores.begin(), cores.end());
        return *this;
    }

    core_affinity &core_affinity::remove(const core_affinity &other) {
        std::unordered_set<core_node, core_hasher> set;
        for (auto core : other.cores) {
            set.emplace(core);
        }
        for (size_t i = 0; i < cores.size(); i++) {
            if (set.count(cores[i]) != 0) {
                cores[i] = cores.back();
                cores.resize(cores.size() - 1);
            }
        }
        std::sort(cores.begin(), cores.end());
        return *this;
    }

    unsigned int core_affinity::num_logical_cores() {
        return static_cast<unsigned int>(sysconf(_SC_NPROCESSORS_ONLN));
    }

    core_affinity core_affinity::group_cores(int node_id, const std::vector<int> &cores) {
        core_affinity affinity;
        for (size_t i = 0; i < cores.size(); i++) {
            core_node node;
            node.index = cores[i];
            node.group = node_id;
            affinity.cores.push_back(node);
        }
        return affinity;
    }

}  // namespace flare
