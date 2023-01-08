
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/



#include "flare/base/profile.h"

#ifdef FLARE_PLATFORM_LINUX
#include <cerrno>
#include <charconv>
#include <filesystem>
#include <functional>
#include <unistd.h>
#include "flare/system/sysinfo.h"
#include "flare/files/filesystem.h"
#include "flare/files/readline_file.h"
#include "flare/strings/str_split.h"
#include "flare/strings/numbers.h"

namespace flare {

#define pageshift(x) ((x)*system_constants::instance().pagesize)

#define LINUX_SYSINFO_TICK2MSEC(s) \
    ((uint64_t)(s) *       \
     ((uint64_t)1000 / (double)system_constants::instance().ticks))

    const char *mock_root = nullptr;

    void set_procfs_root(const char *root) {
        mock_root = root;
    }

    static flare::file_path get_proc_root() {
        if (mock_root) {
            return flare::file_path(mock_root) / "mock" / "linux" / "proc";
        }
        return flare::file_path{"/proc"};

    }

    static void tokenize_file_line_by_line(
            const flare::file_path &name,
            std::function<bool(const std::vector<std::string_view> &)> callback,
            char delim = ' ',
            bool allowEmpty = true) {
        flare::readline_file file;
        auto rs = file.open(name);
        if (!rs.is_ok()) {
            return;
        }
        auto lines = file.lines();
        for (auto line : lines) {
            std::vector<std::string_view> toks;
            if (allowEmpty) {
                toks = flare::string_split(line, by_char(delim), flare::allow_empty());
            } else {
                toks = flare::string_split(line, by_char(delim), flare::skip_empty());
            }
            auto r = callback(toks);
            if (!r) {
                return;
            }
        }
    }


    /// The content of /proc/[pid]/stat (and /proc/[pid]/task/[tid]/stat) file
    /// as described in https://man7.org/linux/man-pages/man5/proc.5.html
    constexpr size_t stat_pid_index = 1;
    constexpr size_t stat_name_index = 2;
    constexpr size_t stat_state_index = 3;
    constexpr size_t stat_ppid_index = 4;
    constexpr size_t stat_tty_index = 7;
    constexpr size_t stat_minor_faults_index = 10;
    constexpr size_t stat_major_faults_index = 12;
    constexpr size_t stat_utime_index = 14;
    constexpr size_t stat_stime_index = 15;
    constexpr size_t stat_priority_index = 18;
    constexpr size_t stat_nice_index = 19;
    constexpr size_t stat_start_time_index = 22;
    constexpr size_t stat_rss_index = 24;
    constexpr size_t stat_processor_index = 39;

    static void sysinfo_tokenize_file_line_by_line(
            flare_pid_t pid,
            const char *filename,
            std::function<bool(const std::vector<std::string_view> &)> callback,
            char delim = ' ') {
        auto name = get_proc_root();
        if (pid) {
            name = name / std::to_string(pid);
        }

        name = name / filename;
        tokenize_file_line_by_line(name, callback, delim, false);
    }

    struct system_constants {
        static system_constants instance() {
            static system_constants inst;
            return inst;
        }

        system_constants()
                : pagesize(getpagesize()),
                  boot_time(get_boot_time()),
                  ticks(sysconf(_SC_CLK_TCK)) {
        }

        static uint64_t get_boot_time() {
            uint64_t ret = 0;
            sysinfo_tokenize_file_line_by_line(
                    0,
                    "stat",
                    [&ret](const auto &vec) {
                        if (vec.size() > 1 && vec.front() == "btime") {
                            ret = std::stoull(std::string(vec[1]));
                            return false;
                        }
                        return true;
                    },
                    ' ');
            return ret;
        }

        const int pagesize;
        const long boot_time;
        const int ticks;
    };


    struct linux_proc_stat_info {
        flare_pid_t pid;
        uint64_t rss;
        uint64_t minor_faults;
        uint64_t major_faults;
        uint64_t ppid;
        int tty;
        int priority;
        int nice;
        uint64_t start_time;
        uint64_t utime;
        uint64_t stime;
        std::string name;
        char state;
        int processor;
    };

    class linux_info : public sysinfo {
    public:
        linux_info() = default;

        ~linux_info() override = default;

        result_status get_mem_info(mem_info &mem) override;

        result_status get_swap(swap_info &swap) override;

        result_status get_cpu(cpu_info &cpu) override;

        result_status get_proc_mem(flare_pid_t pid, proc_mem_info &procmem) override;

        result_status get_proc_state(flare_pid_t pid, proc_state_info &procstate) override;

    protected:

        int get_proc_time(flare_pid_t pid, proc_time_info &proctime) override;

        static result_status proc_stat_read(flare_pid_t pid, linux_proc_stat_info &stat);

        static result_status parse_stat_file(const flare::file_path &name, linux_proc_stat_info &ret);
    };

    result_status linux_info::get_mem_info(mem_info &mem) {
        uint64_t buffers = 0;
        uint64_t cached = 0;
        sysinfo_tokenize_file_line_by_line(
                0,
                "meminfo",
                [&mem, &buffers, &cached](const auto &vec) {
                    if (vec.size() < 2) {
                        return true;
                    }
                    if (vec.front() == "MemTotal") {
                        return flare::simple_atoi(vec[1], &mem.total);
                    }
                    if (vec.front() == "MemFree") {
                        return flare::simple_atoi(vec[1], &mem.free);
                    }
                    if (vec.front() == "Buffers") {
                        return flare::simple_atoi(vec[1], &buffers);
                    }
                    if (vec.front() == "Cached") {
                        return flare::simple_atoi(vec[1], &cached);
                    }

                    return true;
                },
                ':');

        mem.used = mem.total - mem.free;
        auto kern = buffers + cached;
        mem.actual_free = mem.free + kern;
        mem.actual_used = mem.used - kern;
        mem_calc_ram(mem);
        return result_status::success();
    }

    result_status linux_info::get_swap(swap_info &swap) {
        sysinfo_tokenize_file_line_by_line(
                0,
                "meminfo",
                [&swap](const auto &vec) {
                    if (vec.size() < 2) {
                        return true;
                    }
                    if (vec.front() == "SwapTotal") {
                        return flare::simple_atoi(vec[1], &swap.total);
                    }
                    if (vec.front() == "SwapFree") {
                        return flare::simple_atoi(vec[1], &swap.free);
                    }
                    return true;
                },
                ':');

        swap.used = swap.total - swap.free;
        sysinfo_tokenize_file_line_by_line(
                0,
                "vmstat",
                [&swap](const auto &vec) {
                    if (vec.size() < 2) {
                        return true;
                    }

                    if (vec.front() == "pswpin") {
                        flare::simple_atoi(vec[1], &swap.page_in);
                    } else if (vec.front() == "pswpout") {
                        flare::simple_atoi(vec[1], &swap.page_out);
                    } else if (vec.front() == "allocstall") {
                        flare::simple_atoi(vec[1], &swap.allocstall);
                    } else if (vec.front() == "allocstall_dma") {
                        flare::simple_atoi(vec[1], &swap.allocstall_dma);
                    } else if (vec.front() == "allocstall_dma32") {
                        flare::simple_atoi(vec[1], &swap.allocstall_dma32);
                    } else if (vec.front() == "allocstall_normal") {
                        flare::simple_atoi(vec[1], &swap.allocstall_normal);
                    } else if (vec.front() == "allocstall_movable") {
                        flare::simple_atoi(vec[1], &swap.allocstall_movable);
                    }

                    return true;
                },
                ' ');
        return result_status::success();
    }

    result_status linux_info::get_cpu(cpu_info &cpu) {
        int status = ENOENT;
        sysinfo_tokenize_file_line_by_line(
                0,
                "stat",
                [&cpu, &status](const auto &vec) {
                    // The first line in /proc/stat looks like:
                    // cpu user nice system idle iowait irq softirq steal guest
                    // guest_nice (The amount of time, measured in units of
                    // USER_HZ)
                    if (vec.size() < 11) {
                        status = EINVAL;
                        return false;
                    }
                    if (vec.front() == "cpu") {
                        if (vec.size() < 9) {
                            status = EINVAL;
                            return false;
                        }
                        int64_t n;
                        flare::simple_atoi(vec[1], &n);
                        cpu.user = LINUX_SYSINFO_TICK2MSEC(n);
                        flare::simple_atoi(vec[2], &n);
                        cpu.nice = LINUX_SYSINFO_TICK2MSEC(n);
                        flare::simple_atoi(vec[3], &n);
                        cpu.sys = LINUX_SYSINFO_TICK2MSEC(n);
                        flare::simple_atoi(vec[4], &n);
                        cpu.idle = LINUX_SYSINFO_TICK2MSEC(n);
                        flare::simple_atoi(vec[5], &n);
                        cpu.wait = LINUX_SYSINFO_TICK2MSEC(n);
                        flare::simple_atoi(vec[6], &n);
                        cpu.irq = LINUX_SYSINFO_TICK2MSEC(n);
                        flare::simple_atoi(vec[7], &n);
                        cpu.soft_irq = LINUX_SYSINFO_TICK2MSEC(n);
                        flare::simple_atoi(vec[8], &n);
                        cpu.stolen = LINUX_SYSINFO_TICK2MSEC(n);
                        cpu.total = cpu.user + cpu.nice + cpu.sys + cpu.idle +
                                    cpu.wait + cpu.irq + cpu.soft_irq + cpu.stolen;
                        status = 0;
                        return false;
                    }

                    return true;
                },
                ' ');

        return result_status::success();
    }

    result_status linux_info::get_proc_mem(flare_pid_t pid, proc_mem_info &procmem) {
        linux_proc_stat_info pstat;
        auto rs = proc_stat_read(pid, pstat);
        if (rs.is_ok()) {
            return rs;
        }

        procmem.minor_faults = pstat.minor_faults;
        procmem.major_faults = pstat.major_faults;
        procmem.page_faults = procmem.minor_faults + procmem.major_faults;

        sysinfo_tokenize_file_line_by_line(
                pid,
                "statm",
                [&procmem](const auto &vec) {
                    // The format of statm is a single line with the following
                    // numbers (in pages)
                    // size resident shared text lib data dirty
                    if (vec.size() > 2) {
                        procmem.size = pageshift(std::stoull(std::string(vec[0])));
                        procmem.resident =
                                pageshift(std::stoull(std::string(vec[1])));
                        procmem.share = pageshift(std::stoull(std::string(vec[2])));
                        return false;
                    }
                    return true;
                },
                ' ');

        return result_status::success();
    }

    result_status linux_info::get_proc_state(flare_pid_t pid, proc_state_info &procstate) {
        linux_proc_stat_info pstat;
        auto status = proc_stat_read(pid, pstat);
        if (!status.is_ok()) {
            return status;
        }

        // according to the proc manpage pstat.name contains up to 16 characters,
        // and procstate.name is 128 bytes large, but to be on the safe side ;)
        if (pstat.name.length() > sizeof(procstate.name) - 1) {
            pstat.name.resize(sizeof(procstate.name) - 1);
        }
        procstate.name =  pstat.name;
        procstate.state = pstat.state;
        procstate.ppid = pstat.ppid;
        procstate.tty = pstat.tty;
        procstate.priority = pstat.priority;
        procstate.nice = pstat.nice;
        procstate.processor = pstat.processor;

        sysinfo_tokenize_file_line_by_line(
                pid,
                "status",
                [&procstate](const auto &vec) {
                    if (vec.size() > 1 && vec.front() == "Threads") {
                        procstate.threads = std::stoull(std::string(vec[1]));
                        return false;
                    }
                    return true;
                },
                ':');

        return result_status::success();
    }

    result_status linux_info::parse_stat_file(const flare::file_path &name, linux_proc_stat_info &ret) {
        flare::readline_file file;
        file.open(name);
        auto lines = file.lines();
        if (lines.size() != 1) {
            return  result_status(EINVAL,"parse_stat_file(): file " +
                                     name.generic_string() +
                                     " contained multiple lines!");
        }

        auto line = lines[0];
        std::vector<std::string_view> fields = flare::string_split(line, ' ');
        if (fields.size() < stat_processor_index) {
           return result_status(EINVAL,"parse_stat_file(): file " +
                                     name.generic_string() +
                                     " does not contain enough fields");
        }

        // For some stupid reason the /proc files on linux consists of "formatted
        // ASCII" files so that we need to perform text parsing to pick out the
        // correct values (instead of the "binary" mode used on other systems
        // where you could do an ioctl / read and get the struct populated with
        // the correct values.
        // For "stat" this is extra annoying as space is used as the field
        // separator, but the command line can contain a space it is enclosed
        // in () (so using '\n' instead of ' ' would have made it easier to parse
        // :P ).
        while (fields[1].find(')') == std::string_view::npos) {
            fields[1] = {fields[1].data(), fields[1].size() + fields[2].size() + 1};
            auto iter = fields.begin();
            iter++;
            iter++;
            fields.erase(iter);
            if (fields.size() < stat_processor_index) {
                throw std::runtime_error("parse_stat_file(): file " +
                                         name.generic_string() +
                                         " does not contain enough fields");
            }
        }
        // now remove '(' and ')'
        fields[1].remove_prefix(1);
        fields[1].remove_suffix(1);

        // Insert a dummy 0 element so that the index we use map directly
        // to the number specified in
        // https://man7.org/linux/man-pages/man5/proc.5.html
        fields.insert(fields.begin(), "dummy element");
        if(!flare::simple_atoi(fields[stat_pid_index], &ret.pid)) {
            return result_status::from_flare_error(EINVAL);
        }

        ret.name = std::string{fields[stat_name_index].data(),
                               fields[stat_name_index].size()};
        ret.state = fields[stat_state_index].front();
        if(!flare::simple_atoi(fields[stat_ppid_index], &ret.ppid)) {
            return result_status::from_flare_error(EINVAL);
        }

        if(!flare::simple_atoi(fields[stat_tty_index], &ret.tty)) {
            return result_status::from_flare_error(EINVAL);
        }
        if(!flare::simple_atoi(fields[stat_minor_faults_index], &ret.minor_faults)) {
            return result_status::from_flare_error(EINVAL);
        }
        if(!flare::simple_atoi(fields[stat_major_faults_index], &ret.major_faults)) {
            return result_status::from_flare_error(EINVAL);
        }
        int64_t utime;
        if(!flare::simple_atoi(fields[stat_utime_index], &utime)) {
            return result_status::from_flare_error(EINVAL);
        }
        ret.utime = LINUX_SYSINFO_TICK2MSEC(utime);
        int64_t stime;
        if(!flare::simple_atoi(fields[stat_stime_index], &stime)) {
            return result_status::from_flare_error(EINVAL);
        }
        ret.stime = LINUX_SYSINFO_TICK2MSEC(stime);
        if(!flare::simple_atoi(fields[stat_priority_index], &ret.priority)) {
            return result_status::from_flare_error(EINVAL);
        }
        if(!flare::simple_atoi(fields[stat_nice_index], &ret.nice)) {
            return result_status::from_flare_error(EINVAL);
        }
        if(!flare::simple_atoi(fields[stat_start_time_index], &ret.start_time)) {
            return result_status::from_flare_error(EINVAL);
        }
        ret.start_time /= system_constants::instance().ticks;
        ret.start_time += system_constants::instance().boot_time; /* seconds */
        ret.start_time *= 1000; /* milliseconds */
        if(!flare::simple_atoi(fields[stat_rss_index], &ret.rss)) {
            return result_status::from_flare_error(EINVAL);
        }
        if(!flare::simple_atoi(fields[stat_processor_index], &ret.processor)) {
            return result_status::from_flare_error(EINVAL);
        }
        return result_status::success();
    }

    result_status linux_info::proc_stat_read(flare_pid_t pid, linux_proc_stat_info &stat) {
        const auto nm = get_proc_root() / std::to_string(pid) / "stat";
        return parse_stat_file(nm, stat);
    }

    int linux_info::get_proc_time(flare_pid_t pid, proc_time_info &proctime) {
        linux_proc_stat_info pstat;
        const auto status =proc_stat_read(pid, pstat);
        if (!status.is_ok()) {
            return -1;
        }

        proctime.user = pstat.utime;
        proctime.sys = pstat.stime;
        proctime.total = proctime.user + proctime.sys;
        proctime.start_time = pstat.start_time;

        return 0;
    }
    sysinfo *sysinfo::get_instance() {
        static linux_info info;
        return reinterpret_cast<sysinfo *>(&info);
    }

}  // namespace flare
#endif  // FLARE_PLATFORM_LINUX
