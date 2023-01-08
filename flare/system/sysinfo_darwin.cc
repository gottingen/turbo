
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include "flare/base/profile.h"

#ifdef FLARE_PLATFORM_OSX

#include <errno.h>
#include <libproc.h>
#include <mach-o/dyld.h>
#include <mach/host_info.h>
#include <mach/kern_return.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/mach_port.h>
#include <mach/mach_traps.h>
#include <mach/shared_region.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <mach/thread_info.h>
#include <mach/vm_map.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include "flare/system/sysinfo.h"
#include "flare/times/time.h"

namespace flare {

#define NMIB(mib) (sizeof(mib) / sizeof(mib[0]))
#define SYSINFO_TICK2MSEC(s) \
    ((uint64_t)(s) * ((uint64_t)1000 / (double)info->ticks))

#define tval2msec(tval) \
    ((tval.seconds * 1000) + (tval.microseconds / 1000))

#define SYSINFO_NSEC2MSEC(s) ((uint64_t)(s) / ((uint64_t)1000000L))

#define SYSINFO_PROC_STATE_SLEEP 'S'
#define SYSINFO_PROC_STATE_RUN 'R'
#define SYSINFO_PROC_STATE_STOP 'T'
#define SYSINFO_PROC_STATE_ZOMBIE 'Z'
#define SYSINFO_PROC_STATE_IDLE 'D'

    static const char thread_states[] = {
            /*0*/ '-',
            /*1*/ SYSINFO_PROC_STATE_RUN,
            /*2*/ SYSINFO_PROC_STATE_ZOMBIE,
            /*3*/ SYSINFO_PROC_STATE_SLEEP,
            /*4*/ SYSINFO_PROC_STATE_IDLE,
            /*5*/ SYSINFO_PROC_STATE_STOP,
            /*6*/ SYSINFO_PROC_STATE_STOP,
            /*7*/ '?'};


    int darwin_proc_cpu_type(flare_pid_t pid, cpu_type_t *type);

    mach_vm_size_t darwin_shared_region_size(cpu_type_t type);

    int thread_state_get(thread_basic_info_data_t *info);

    class darwin_info : public sysinfo {
    public:
        darwin_info();

        ~darwin_info() override = default;

        result_status get_mem_info(mem_info &mem) override;

        result_status get_swap(swap_info &swap) override;

        result_status get_cpu(cpu_info &cpu) override;

        result_status get_proc_mem(flare_pid_t pid, proc_mem_info &procmem) override;

        result_status get_proc_state(flare_pid_t pid, proc_state_info &procstate) override;

    protected:

        int get_proc_time(flare_pid_t pid, proc_time_info &proctime) override;

        result_status get_vmstat(vm_statistics_data_t *vmstat);

        static std::pair<int, kinfo_proc> get_pinfo(flare_pid_t pid);

        static int get_proc_threads(flare_pid_t pid, proc_state_info &procstate);

        const int ticks;
        int pagesize;
        mach_port_t mach_port;
    };

    darwin_info::darwin_info()
            : ticks(sysconf(_SC_CLK_TCK)),
              pagesize(getpagesize()),
              mach_port(mach_host_self()) {

    }

    result_status darwin_info::get_mem_info(mem_info &mem) {
        uint64_t kern = 0;
        vm_statistics_data_t vmstat;
        uint64_t mem_total;
        int mib[2];
        size_t len;
        mib[0] = CTL_HW;

        mib[1] = HW_PAGESIZE;
        len = sizeof(pagesize);
        if (sysctl(mib, NMIB(mib), &pagesize, &len, nullptr, 0) < 0) {
            return result_status::from_flare_error(errno);
        }

        mib[1] = HW_MEMSIZE;
        len = sizeof(mem_total);
        if (sysctl(mib, NMIB(mib), &mem_total, &len, nullptr, 0) < 0) {
            return result_status::from_flare_error(errno);
        }

        mem.total = mem_total;

        auto status = get_vmstat(&vmstat);
        if (!status.is_ok()) {
            return status;
        }

        mem.free = vmstat.free_count;
        mem.free *= pagesize;
        kern = vmstat.inactive_count;
        kern *= pagesize;

        mem.used = mem.total - mem.free;

        mem.actual_free = mem.free + kern;
        mem.actual_used = mem.used - kern;
        mem_calc_ram(mem);

        return result_status::success();
    }

    result_status darwin_info::get_swap(swap_info &swap) {
        struct xsw_usage sw_usage;
        size_t size = sizeof(sw_usage);
        int mib[] = {CTL_VM, VM_SWAPUSAGE};

        if (sysctl(mib, NMIB(mib), &sw_usage, &size, nullptr, 0) != 0) {
            return result_status::from_flare_error(errno);
        }

        swap.total = sw_usage.xsu_total;
        swap.used = sw_usage.xsu_used;
        swap.free = sw_usage.xsu_avail;

        vm_statistics_data_t vmstat;
        const auto status = get_vmstat(&vmstat);
        if (!status.is_ok()) {
            return status;
        }
        swap.page_in = vmstat.pageins;
        swap.page_out = vmstat.pageouts;
        return result_status::success();
    }

    result_status darwin_info::get_cpu(cpu_info &cpu) {
        kern_return_t status;
        mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
        host_cpu_load_info_data_t cpuload;

        status = host_statistics(
                mach_port, HOST_CPU_LOAD_INFO, (host_info_t) &cpuload, &count);

        if (status != KERN_SUCCESS) {
            return result_status::from_flare_error(errno);
        }

        auto *info = this;
        cpu.user = SYSINFO_TICK2MSEC(cpuload.cpu_ticks[CPU_STATE_USER]);
        cpu.sys = SYSINFO_TICK2MSEC(cpuload.cpu_ticks[CPU_STATE_SYSTEM]);
        cpu.idle = SYSINFO_TICK2MSEC(cpuload.cpu_ticks[CPU_STATE_IDLE]);
        cpu.nice = SYSINFO_TICK2MSEC(cpuload.cpu_ticks[CPU_STATE_NICE]);
        cpu.total = cpu.user + cpu.nice + cpu.sys + cpu.idle;
        return result_status::success();

    }

    result_status darwin_info::get_proc_mem(flare_pid_t pid, proc_mem_info &procmem) {
        mach_port_t task, self = mach_task_self();
        kern_return_t status;
        task_basic_info_data_t info;
        task_events_info_data_t events;
        mach_msg_type_number_t count;
        struct proc_taskinfo pti;
        struct proc_regioninfo pri;

        int sz = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));
        if (sz == sizeof(pti)) {
            procmem.size = pti.pti_virtual_size;
            procmem.resident = pti.pti_resident_size;
            procmem.page_faults = pti.pti_faults;

            sz = proc_pidinfo(pid, PROC_PIDREGIONINFO, 0, &pri, sizeof(pri));
            if (sz == sizeof(pri)) {
                if (pri.pri_share_mode == SM_EMPTY) {
                    mach_vm_size_t shared_size;
                    cpu_type_t cpu_type;

                    if (darwin_proc_cpu_type(pid, &cpu_type) == 0) {
                        shared_size = darwin_shared_region_size(cpu_type);
                    } else {
                        shared_size =
                                SHARED_REGION_SIZE_I386; /* assume 32-bit x86|ppc */
                    }
                    if (procmem.size > shared_size) {
                        procmem.size -= shared_size; /* SIGAR-123 */
                    }
                }
            }
            return result_status::success();
        }

        status = task_for_pid(self, pid, &task);

        if (status != KERN_SUCCESS) {
            return result_status::from_flare_error(errno);
        }

        count = TASK_BASIC_INFO_COUNT;
        status = task_info(task, TASK_BASIC_INFO, (task_info_t) &info, &count);
        if (status != KERN_SUCCESS) {
            return result_status::from_flare_error(errno);
        }

        count = TASK_EVENTS_INFO_COUNT;
        status = task_info(task, TASK_EVENTS_INFO, (task_info_t) &events, &count);
        if (status == KERN_SUCCESS) {
            procmem.page_faults = events.faults;
        }

        if (task != self) {
            mach_port_deallocate(self, task);
        }

        procmem.size = info.virtual_size;
        procmem.resident = info.resident_size;
        return result_status::success();
    }

    result_status darwin_info::get_proc_state(flare_pid_t pid, proc_state_info &procstate) {
        const auto[status, pinfo] = get_pinfo(pid);
        if (status != 0) {
            return flare::result_status::from_flare_error(status);
        }
        int state = pinfo.kp_proc.p_stat;

        procstate.name = pinfo.kp_proc.p_comm;
        procstate.ppid = pinfo.kp_eproc.e_ppid;
        procstate.priority = pinfo.kp_proc.p_priority;
        procstate.nice = pinfo.kp_proc.p_nice;

        auto st = get_proc_threads(pid, procstate);
        if (st == 0) {
            return result_status::success();
        }

        switch (state) {
            case SIDL:
                procstate.state = 'D';
                break;
            case SRUN:
#ifdef SONPROC
                case SONPROC:
#endif
                procstate.state = 'R';
                break;
            case SSLEEP:
                procstate.state = 'S';
                break;
            case SSTOP:
                procstate.state = 'T';
                break;
            case SZOMB:
                procstate.state = 'Z';
                break;
            default:
                procstate.state = '?';
                break;
        }
        return result_status::success();
    }

    result_status darwin_info::get_vmstat(vm_statistics_data_t *vmstat) {
        mach_msg_type_number_t count = sizeof(*vmstat) / sizeof(integer_t);
        const auto status = host_statistics(
                mach_port, HOST_VM_INFO, (host_info_t)vmstat, &count);

        if (status == KERN_SUCCESS) {
            return result_status::success();
        } else {
            return result_status::from_flare_error(errno);
        }
    }

    std::pair<int, kinfo_proc> darwin_info::get_pinfo(flare_pid_t pid) {
        int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, 0};
        mib[3] = pid;

        kinfo_proc pinfo = {};
        size_t len = sizeof(pinfo);

        if (sysctl(mib, NMIB(mib), &pinfo, &len, nullptr, 0) < 0) {
            return {errno, {}};
        }

        return {0, pinfo};
    }

    int darwin_info::get_proc_threads(flare_pid_t pid, proc_state_info &procstate) {
        mach_port_t task, self = mach_task_self();
        kern_return_t status;
        thread_array_t threads;
        mach_msg_type_number_t count, i;
        int state = TH_STATE_HALTED + 1;

        status = task_for_pid(self, pid, &task);
        if (status != KERN_SUCCESS) {
            return errno;
        }

        status = task_threads(task, &threads, &count);
        if (status != KERN_SUCCESS) {
            return errno;
        }

        procstate.threads = count;

        for (i = 0; i < count; i++) {
            mach_msg_type_number_t info_count = THREAD_BASIC_INFO_COUNT;
            thread_basic_info_data_t info;

            status = thread_info(threads[i],
                                 THREAD_BASIC_INFO,
                                 (thread_info_t) &info,
                                 &info_count);
            if (status == KERN_SUCCESS) {
                int tstate = thread_state_get(&info);
                if (tstate < state) {
                    state = tstate;
                }
            }
        }

        vm_deallocate(self, (vm_address_t) threads, sizeof(thread_t) * count);

        procstate.state = thread_states[state];
        return 0;
    }

    static int get_proc_times(flare_pid_t pid, proc_time_info *time) {
        unsigned int count;
        time_value_t utime = {0, 0}, stime = {0, 0};
        task_basic_info_data_t ti;
        task_thread_times_info_data_t tti;
        task_port_t task, self;
        kern_return_t status;

        struct proc_taskinfo pti;
        int sz = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));

        if (sz == sizeof(pti)) {
            time->user = SYSINFO_NSEC2MSEC(pti.pti_total_user);
            time->sys = SYSINFO_NSEC2MSEC(pti.pti_total_system);
            time->total = time->user + time->sys;
            return 0;
        }

        self = mach_task_self();
        status = task_for_pid(self, pid, &task);
        if (status != KERN_SUCCESS) {
            return errno;
        }

        count = TASK_BASIC_INFO_COUNT;
        status = task_info(task, TASK_BASIC_INFO, (task_info_t) &ti, &count);
        if (status != KERN_SUCCESS) {
            if (task != self) {
                mach_port_deallocate(self, task);
            }
            return errno;
        }

        count = TASK_THREAD_TIMES_INFO_COUNT;
        status = task_info(task, TASK_THREAD_TIMES_INFO, (task_info_t) &tti, &count);
        if (status != KERN_SUCCESS) {
            if (task != self) {
                mach_port_deallocate(self, task);
            }
            return errno;
        }

        time_value_add(&utime, &ti.user_time);
        time_value_add(&stime, &ti.system_time);
        time_value_add(&utime, &tti.user_time);
        time_value_add(&stime, &tti.system_time);

        time->user = tval2msec(utime);
        time->sys = tval2msec(stime);
        time->total = time->user + time->sys;

        return 0;
    }

    int darwin_info::get_proc_time(flare_pid_t pid, proc_time_info &proctime) {
        const auto[status, pinfo] = get_pinfo(pid);
        if (status != 0) {
            return status;
        }

        int st = get_proc_times(pid, &proctime);
        if (st != 0) {
            return st;
        }

        proctime.start_time = flare::time_point::from_timeval(pinfo.kp_proc.p_starttime).to_unix_millis();
        return 0;
    }

    sysinfo *sysinfo::get_instance() {
        static darwin_info info;
        return reinterpret_cast<sysinfo *>(&info);
    }

    int darwin_proc_cpu_type(flare_pid_t pid, cpu_type_t *type) {
        int status;
        int mib[CTL_MAXNAME];
        size_t len, miblen = NMIB(mib);

        status = sysctlnametomib("sysctl.proc_cputype", mib, &miblen);
        if (status != 0) {
            return status;
        }

        mib[miblen] = pid;
        len = sizeof(*type);
        return sysctl(mib, miblen + 1, type, &len, nullptr, 0);
    }


    /* shared memory region size for the given cpu_type_t */
    mach_vm_size_t darwin_shared_region_size(cpu_type_t type) {
        switch (type) {
            case CPU_TYPE_ARM:
                return SHARED_REGION_SIZE_ARM;
            case CPU_TYPE_POWERPC:
                return SHARED_REGION_SIZE_PPC;
            case CPU_TYPE_POWERPC64:
                return SHARED_REGION_SIZE_PPC64;
            case CPU_TYPE_I386:
                return SHARED_REGION_SIZE_I386;
            case CPU_TYPE_X86_64:
                return SHARED_REGION_SIZE_X86_64;
            default:
                return SHARED_REGION_SIZE_I386; /* assume 32-bit x86|ppc */
        }
    }


    int thread_state_get(thread_basic_info_data_t *info) {
        switch (info->run_state) {
            case TH_STATE_RUNNING:
                return 1;
            case TH_STATE_UNINTERRUPTIBLE:
                return 2;
            case TH_STATE_WAITING:
                return (info->sleep_time > 20) ? 4 : 3;
            case TH_STATE_STOPPED:
                return 5;
            case TH_STATE_HALTED:
                return 6;
            default:
                return 7;
        }
    }

}  // namespace flare
#endif  // FLARE_PLATFORM_OSX
