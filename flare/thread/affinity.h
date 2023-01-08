
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_THREAD_AFFINITY_H_
#define FLARE_THREAD_AFFINITY_H_

#include <vector>
#include <cstdint>
#include <memory>

namespace flare {

    // core_node identifies a logical processor unit.
    // How a core is identified varies by platform.
    struct core_node {
        core_node() : group(-1), index(-1) {}

        ~core_node() = default;

        int32_t group;
        int32_t index;

        // Comparison functions
        inline bool operator==(const core_node &) const;

        inline bool operator<(const core_node &) const;
    };


    // core_affinity holds the affinity mask for a thread - a description of what cores
    // the thread is allowed to run on.
    struct core_affinity {
        // supported is true if flare supports controlling thread affinity for this
        // platform.
#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__)) || \
    defined(__FreeBSD__)
        static constexpr bool supported = true;
#else
        static constexpr bool supported = false;
#endif

        // affinity_policy is an interface that provides a get() method for returning an
        // core_affinity for the given thread by id.
        class affinity_policy {
        public:
            virtual ~affinity_policy() {}

            // any_of() returns a affinity_policy that returns an core_affinity for a number of
            // available cores in affinity.
            //
            // Windows requires that each thread is only associated with a
            // single affinity group, so the affinity_policy's returned affinity will contain
            // cores all from the same group.
            static std::shared_ptr<affinity_policy> any_of(core_affinity &&affinity);

            // one_of() returns a affinity_policy that returns an affinity with a single enabled
            // core from affinity. The single enabled core in the affinity_policy's returned
            // affinity is:
            //      affinity[threadId % affinity.count()]
            static std::shared_ptr<affinity_policy> one_of(
                    core_affinity &&affinity);

            // get() returns the thread core_affinity for the for the given thread by id.
            virtual core_affinity get(uint32_t threadId) const = 0;
        };

        core_affinity();

        core_affinity(core_affinity &&);

        core_affinity(const core_affinity &) = default;

        core_affinity &operator=(const core_affinity &) = default;

        // all() returns an core_affinity with all the cores available to the process.
        static core_affinity all();

        core_affinity(std::initializer_list<core_node>);

        // count() returns the number of enabled cores in the affinity.
        size_t count() const;

        // operator[] returns the i'th enabled core from this affinity.
        core_node operator[](size_t index) const;

        // add() adds the cores from the given affinity to this affinity.
        // This affinity is returned to allow for fluent calls.
        core_affinity &add(const core_affinity &);

        // remove() removes the cores from the given affinity from this affinity.
        // This affinity is returned to allow for fluent calls.
        core_affinity &remove(const core_affinity &);

        // num_logical_cores() returns the number of available logical CPU cores for
        // the system.
        static unsigned int num_logical_cores();

        static core_affinity group_cores(int node_id, const std::vector<int> &cores);

    private:

        std::vector<core_node> cores;
    };

    // Comparison functions
    bool core_node::operator==(const core_node &other) const {
        return index == other.index;
    }

    bool core_node::operator<(const core_node &other) const {
        return index < other.index;
    }

}  // namespace flare

#endif  // FLARE_THREAD_AFFINITY_H_
