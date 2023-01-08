
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#ifndef FLARE_SYSTEM_SYSINFO_H_
#define FLARE_SYSTEM_SYSINFO_H_

#include <string>
#include "flare/base/result_status.h"
#include "flare/container/flat_hash_map.h"

namespace flare {

    typedef pid_t flare_pid_t;

    struct mem_info {
        uint64_t ram{0};
        uint64_t total{0};
        uint64_t used{0};
        uint64_t free{0};
        uint64_t actual_used{0};
        uint64_t actual_free{0};
        double used_percent{0.0};
        double free_percent{0.0};
    };

    struct swap_info {
        uint64_t total{0};
        uint64_t used{0};
        uint64_t free{0};
        uint64_t page_in{0};
        uint64_t page_out{0};
        uint64_t allocstall{0}; /* up until 4.10 */
        uint64_t allocstall_dma{0}; /* 4.10 onwards */
        uint64_t allocstall_dma32{0}; /* 4.10 onwards */
        uint64_t allocstall_normal{0}; /* 4.10 onwards */
        uint64_t allocstall_movable{0}; /* 4.10 onwards */
    };

    struct cpu_info {
        uint64_t user{0};
        uint64_t sys{0};
        uint64_t nice{0};
        uint64_t idle{0};
        uint64_t wait{0};
        uint64_t irq{0};
        uint64_t soft_irq{0};
        uint64_t stolen{0};
        uint64_t total{0};
    };

    struct proc_mem_info {
        uint64_t size;
        uint64_t resident;
        uint64_t share;
        uint64_t minor_faults;
        uint64_t major_faults;
        uint64_t page_faults;
    };


    struct proc_state_info {
        std::string name;
        char state;
        flare_pid_t ppid;
        int tty;
        int priority;
        int nice;
        int processor;
        uint64_t threads;
    };


    struct proc_cpu_info {
        uint64_t start_time;
        uint64_t user;
        uint64_t sys;
        uint64_t total;
        uint64_t last_time;
        double percent;
    };

    struct proc_time_info {
        uint64_t start_time;
        uint64_t user;
        uint64_t sys;
        uint64_t total;
    };

    void set_procfs_root(const char *root);

    class sysinfo {
    public:
        virtual ~sysinfo() = default;

        virtual result_status get_mem_info(mem_info &minfo) = 0;

        virtual result_status get_swap(swap_info &minfo) = 0;

        virtual result_status get_cpu(cpu_info &minfo) = 0;

        virtual result_status get_proc_mem(flare_pid_t pid, proc_mem_info &minfo) = 0;

        virtual result_status get_proc_state(flare_pid_t pid, proc_state_info &minfo) = 0;

        result_status get_proc_cpu(flare_pid_t pid, proc_cpu_info &minfo);

        static size_t get_page_size();

        static double nominal_cpu_frequency();

        static int num_cpus();

        static const std::string &user_name();

        static const std::string &get_host_name();

        static pid_t get_tid();

        static int32_t get_main_thread_pid();

        static bool pid_has_changed();

    public:
        static sysinfo *get_instance();

    protected:

        virtual int get_proc_time(flare_pid_t pid, proc_time_info &proctime) = 0;

        flat_hash_map<flare_pid_t, proc_cpu_info> process_cache;

        static void mem_calc_ram(mem_info &mem);
    };

}  // namespace flare

#endif  // FLARE_SYSTEM_SYSINFO_H_
