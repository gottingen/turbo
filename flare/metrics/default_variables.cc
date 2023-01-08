
#ifndef UNIT_TEST
#include <unistd.h>                        // getpagesize
#include <sys/types.h>
#include <sys/resource.h>                  // getrusage
#include <dirent.h>                        // dirent
#include <iomanip>                         // setw

#if defined(__APPLE__)

#include <libproc.h>
#include <sys/resource.h>

#else
#endif

#include "flare/files/filesystem.h"
#include "flare/times/time.h"
#include "flare/base/singleton_on_pthread_once.h"
#include "flare/base/scoped_lock.h"
#include "flare/files/sequential_read_file.h"
#include "flare/system/process.h"
#include "flare/metrics/gauge.h"
#include "flare/base/static_atomic.h"

namespace flare {

    template<class T, class M>
    M get_member_type(M T::*);

#define VARIABLE_MEMBER_TYPE(member) decltype(flare::get_member_type(member))

    int do_link_default_variables = 0;
    const int64_t CACHED_INTERVAL_US = 100000L; // 100ms

// ======================================
    struct ProcStat {
        int pid;
        //std::string comm;
        char state;
        int ppid;
        int pgrp;
        int session;
        int tty_nr;
        int tpgid;
        unsigned flags;
        unsigned long minflt;
        unsigned long cminflt;
        unsigned long majflt;
        unsigned long cmajflt;
        unsigned long utime;
        unsigned long stime;
        unsigned long cutime;
        unsigned long cstime;
        long priority;
        long nice;
        long num_threads;
    };

    static bool read_proc_status(ProcStat &stat) {
        stat = ProcStat();
        errno = 0;
#if defined(FLARE_PLATFORM_LINUX)
        // Read status from /proc/self/stat. Information from `man proc' is out of date,
        // see http://man7.org/linux/man-pages/man5/proc.5.html
        flare::sequential_read_file file;
        auto status = file.open("/proc/self/stat");
        if (!status.is_ok()) {
            FLARE_PLOG_ONCE(WARNING) << "Fail to open /proc/self/stat";
            return false;
        }
        std::string content;
        file.read(&content);
        if (sscanf(content.c_str(), "%d %*s %c "
                   "%d %d %d %d %d "
                   "%u %lu %lu %lu "
                   "%lu %lu %lu %lu %lu "
                   "%ld %ld %ld",
                   &stat.pid, &stat.state,
                   &stat.ppid, &stat.pgrp, &stat.session, &stat.tty_nr, &stat.tpgid,
                   &stat.flags, &stat.minflt, &stat.cminflt, &stat.majflt,
                   &stat.cmajflt, &stat.utime, &stat.stime, &stat.cutime, &stat.cstime,
                   &stat.priority, &stat.nice, &stat.num_threads) != 19) {
            FLARE_PLOG(WARNING) << "Fail to sscanf";
            return false;
        }
        return true;
#elif defined(FLARE_PLATFORM_OSX)
        // TODO(zhujiashun): get remaining state in MacOS.
        memset(&stat, 0, sizeof(stat));
        static pid_t pid = getpid();
        std::ostringstream oss;
        char cmdbuf[128];
        snprintf(cmdbuf, sizeof(cmdbuf),
                 "ps -p %ld -o pid,ppid,pgid,sess"
                 ",tpgid,flags,pri,nice | tail -n1", (long) pid);
        if (flare::read_command_output(oss, cmdbuf) != 0) {
            FLARE_LOG(ERROR) << "Fail to read stat";
            return -1;
        }
        const std::string &result = oss.str();
        if (sscanf(result.c_str(), "%d %d %d %d"
                                   "%d %u %ld %ld",
                   &stat.pid, &stat.ppid, &stat.pgrp, &stat.session,
                   &stat.tpgid, &stat.flags, &stat.priority, &stat.nice) != 8) {
            FLARE_PLOG(WARNING) << "Fail to sscanf";
            return false;
        }
        return true;
#else
        return false;
#endif
    }

// Reduce pressures to functions to get system metrics.
    template<typename T>
    class CachedReader {
    public:
        CachedReader() : _mtime_us(0) {
            FLARE_CHECK_EQ(0, pthread_mutex_init(&_mutex, nullptr));
        }

        ~CachedReader() {
            pthread_mutex_destroy(&_mutex);
        }

        // NOTE: may return a volatile value that may be overwritten at any time.
        // This is acceptable right now. Both 32-bit and 64-bit numbers are atomic
        // to fetch in 64-bit machines and the code inside
        // this .cpp utilizing this class generally return a struct with 32-bit
        // and 64-bit numbers.
        template<typename ReadFn>
        static const T &get_value(const ReadFn &fn) {
            CachedReader *p = flare::get_leaky_singleton<CachedReader>();
            const int64_t now = flare::get_current_time_micros();
            if (now > p->_mtime_us + CACHED_INTERVAL_US) {
                pthread_mutex_lock(&p->_mutex);
                if (now > p->_mtime_us + CACHED_INTERVAL_US) {
                    p->_mtime_us = now;
                    pthread_mutex_unlock(&p->_mutex);
                    // don't run fn inside lock otherwise a slow fn may
                    // block all concurrent variable dumppers. (e.g. /vars)
                    T result;
                    if (fn(&result)) {
                        pthread_mutex_lock(&p->_mutex);
                        p->_cached = result;
                    } else {
                        pthread_mutex_lock(&p->_mutex);
                    }
                }
                pthread_mutex_unlock(&p->_mutex);
            }
            return p->_cached;
        }

    private:
        int64_t _mtime_us;
        pthread_mutex_t _mutex;
        T _cached;
    };

    class ProcStatReader {
    public:
        bool operator()(ProcStat *stat) const {
            return read_proc_status(*stat);
        }

        template<typename T, size_t offset>
        static T get_field(void *) {
            return *(T *) ((char *) &CachedReader<ProcStat>::get_value(
                    ProcStatReader()) + offset);
        }
    };

#define VARIABLE_DEFINE_PROC_STAT_FIELD(field)                              \
    status_gauge<VARIABLE_MEMBER_TYPE(&ProcStat::field)> g_##field(        \
        ProcStatReader::get_field<VARIABLE_MEMBER_TYPE(&ProcStat::field),   \
        offsetof(ProcStat, field)>, nullptr);

#define VARIABLE_DEFINE_PROC_STAT_FIELD2(field, name)                       \
    status_gauge<VARIABLE_MEMBER_TYPE(&ProcStat::field)> g_##field(        \
        name,                                                           \
        ProcStatReader::get_field<VARIABLE_MEMBER_TYPE(&ProcStat::field),   \
        offsetof(ProcStat, field)>, nullptr);

// ==================================================

    struct ProcMemory {
        long size;      // total program size
        long resident;  // resident set size
        long share;     // shared pages
        long trs;       // text (code)
        long lrs;       // library
        long drs;       // data/stack
        long dt;        // dirty pages
    };

    static bool read_proc_memory(ProcMemory &m) {
        m = ProcMemory();
        errno = 0;
#if defined(FLARE_PLATFORM_LINUX)
        flare::sequential_read_file file;
        auto status = file.open("/proc/self/statm");
        if (!status.is_ok()) {
            FLARE_PLOG_ONCE(WARNING) << "Fail to open /proc/self/statm";
            return false;
        }
        std::string content;
        file.read(&content);
        if (sscanf(content.c_str(), "%ld %ld %ld %ld %ld %ld %ld",
                   &m.size, &m.resident, &m.share,
                   &m.trs, &m.lrs, &m.drs, &m.dt) != 7) {
            FLARE_PLOG(WARNING) << "Fail to sscanf /proc/self/statm";
            return false;
        }
        return true;
#elif defined(FLARE_PLATFORM_OSX)
        // TODO(zhujiashun): get remaining memory info in MacOS.
        memset(&m, 0, sizeof(m));
        static pid_t pid = getpid();
        static int64_t pagesize = getpagesize();
        std::ostringstream oss;
        char cmdbuf[128];
        snprintf(cmdbuf, sizeof(cmdbuf), "ps -p %ld -o rss=,vsz=", (long) pid);
        if (flare::read_command_output(oss, cmdbuf) != 0) {
            FLARE_LOG(ERROR) << "Fail to read memory state";
            return -1;
        }
        const std::string &result = oss.str();
        if (sscanf(result.c_str(), "%ld %ld", &m.resident, &m.size) != 2) {
            FLARE_PLOG(WARNING) << "Fail to sscanf";
            return false;
        }
        // resident and size in Kbytes
        m.resident = m.resident * 1024 / pagesize;
        m.size = m.size * 1024 / pagesize;
        return true;
#else
        return false;
#endif
    }

    class ProcMemoryReader {
    public:
        bool operator()(ProcMemory *stat) const {
            return read_proc_memory(*stat);
        };

        template<typename T, size_t offset>
        static T get_field(void *) {
            static int64_t pagesize = getpagesize();
            return *(T *) ((char *) &CachedReader<ProcMemory>::get_value(
                    ProcMemoryReader()) + offset) * pagesize;
        }
    };

#define VARIABLE_DEFINE_PROC_MEMORY_FIELD(field, name)                      \
    status_gauge<VARIABLE_MEMBER_TYPE(&ProcMemory::field)> g_##field(      \
        name,                                                           \
        ProcMemoryReader::get_field<VARIABLE_MEMBER_TYPE(&ProcMemory::field), \
        offsetof(ProcMemory, field)>, nullptr);

// ==================================================

    struct LoadAverage {
        double loadavg_1m;
        double loadavg_5m;
        double loadavg_15m;
    };

    static bool read_load_average(LoadAverage &m) {
#if defined(FLARE_PLATFORM_LINUX)
        flare::sequential_read_file file;
        auto status = file.open("/proc/loadavg");
        if (!status.is_ok()) {
            FLARE_PLOG_ONCE(WARNING) << "Fail to open /proc/loadavg";
            return false;
        }
        m = LoadAverage();
        errno = 0;
        std::string content;
        file.read(&content);
        if (sscanf(content.c_str(), "%lf %lf %lf",
                   &m.loadavg_1m, &m.loadavg_5m, &m.loadavg_15m) != 3) {
            FLARE_PLOG(WARNING) << "Fail to fscanf";
            return false;
        }
        return true;
#elif defined(FLARE_PLATFORM_OSX)
        std::ostringstream oss;
        if (flare::read_command_output(oss, "sysctl -n vm.loadavg") != 0) {
            FLARE_LOG(ERROR) << "Fail to read loadavg";
            return -1;
        }
        const std::string &result = oss.str();
        if (sscanf(result.c_str(), "{ %lf %lf %lf }",
                   &m.loadavg_1m, &m.loadavg_5m, &m.loadavg_15m) != 3) {
            FLARE_PLOG(WARNING) << "Fail to sscanf";
            return false;
        }
        return true;

#else
        return false;
#endif
    }

    class LoadAverageReader {
    public:
        bool operator()(LoadAverage *stat) const {
            return read_load_average(*stat);
        };

        template<typename T, size_t offset>
        static T get_field(void *) {
            return *(T *) ((char *) &CachedReader<LoadAverage>::get_value(
                    LoadAverageReader()) + offset);
        }
    };

#define VARIABLE_DEFINE_LOAD_AVERAGE_FIELD(field, name)                     \
    status_gauge<VARIABLE_MEMBER_TYPE(&LoadAverage::field)> g_##field(     \
        name,                                                           \
        LoadAverageReader::get_field<VARIABLE_MEMBER_TYPE(&LoadAverage::field), \
        offsetof(LoadAverage, field)>, nullptr);

// ==================================================

    static int get_fd_count(int limit) {
#if defined(FLARE_PLATFORM_LINUX)
        std::error_code ec;
        flare::directory_iterator di("/proc/self/fd", ec);
        if (ec) {
            FLARE_PLOG(WARNING) << "Fail to open /proc/self/fd";
            return -1;
        }

        // Have to limit the scaning which consumes a lot of CPU when #fd
        // are huge (100k+)
        int count = 0;
        flare::directory_iterator endDi;
        for (; di != endDi && count <= limit + 3; ++count, ++di) {}
        return count - 3; /* skipped ., .. and the fd in di*/

#elif defined(FLARE_PLATFORM_OSX)
        // TODO(zhujiashun): following code will cause core dump with some
        // probability under mac when program exits. Fix it.
        /*
        static pid_t pid = getpid();
        std::ostringstream oss;
        char cmdbuf[128];
        snprintf(cmdbuf, sizeof(cmdbuf),
                "lsof -p %ld | grep -v \"txt\" | wc -l", (long)pid);
        if (flare::read_command_output(oss, cmdbuf) != 0) {
            FLARE_LOG(ERROR) << "Fail to read open files";
            return -1;
        }
        const std::string& result = oss.str();
        int count = 0;
        if (sscanf(result.c_str(), "%d", &count) != 1) {
            FLARE_PLOG(WARNING) << "Fail to sscanf";
            return -1;
        }
        // skipped . and first column line
        count = count - 2;
        return std::min(count, limit);
        */
        return 0;
#else
        return 0;
#endif
    }

    extern status_gauge<int> g_fd_num;

    const int MAX_FD_SCAN_COUNT = 10003;
    static flare::static_atomic<bool> s_ever_reached_fd_scan_limit = FLARE_STATIC_ATOMIC_INIT(false);

    class FdReader {
    public:
        bool operator()(int *stat) const {
            if (s_ever_reached_fd_scan_limit.load(std::memory_order_relaxed)) {
                // Never update the count again.
                return false;
            }
            const int count = get_fd_count(MAX_FD_SCAN_COUNT);
            if (count < 0) {
                return false;
            }
            if (count == MAX_FD_SCAN_COUNT - 2
                && s_ever_reached_fd_scan_limit.exchange(
                    true, std::memory_order_relaxed) == false) {
                // Rename the variable to notify user.
                g_fd_num.hide();
                g_fd_num.expose("process_fd_num_too_many", "");
            }
            *stat = count;
            return true;
        }
    };

    static int print_fd_count(void *) {
        return CachedReader<int>::get_value(FdReader());
    }

// ==================================================
    struct ProcIO {
        // number of bytes the process read, using any read-like system call (from
        // files, pipes, tty...).
        size_t rchar;

        // number of bytes the process wrote using any write-like system call.
        size_t wchar;

        // number of read-like system call invocations that the process performed.
        size_t syscr;

        // number of write-like system call invocations that the process performed.
        size_t syscw;

        // number of bytes the process directly read from disk.
        size_t read_bytes;

        // number of bytes the process originally dirtied in the page-cache
        // (assuming they will go to disk later).
        size_t write_bytes;

        // number of bytes the process "un-dirtied" - e.g. using an "ftruncate"
        // call that truncated pages from the page-cache.
        size_t cancelled_write_bytes;
    };

    static bool read_proc_io(ProcIO *s) {
#if defined(FLARE_PLATFORM_LINUX)
        flare::sequential_read_file file;
        auto status = file.open("/proc/self/io");
        if (!status.is_ok()) {
            FLARE_PLOG_ONCE(WARNING) << "Fail to open /proc/self/io";
            return false;
        }
        errno = 0;
        std::string content;
        file.read(&content);
        if (sscanf(content.c_str(), "%*s %lu %*s %lu %*s %lu %*s %lu %*s %lu %*s %lu %*s %lu",
                   &s->rchar, &s->wchar, &s->syscr, &s->syscw,
                   &s->read_bytes, &s->write_bytes, &s->cancelled_write_bytes)
            != 7) {
            FLARE_PLOG(WARNING) << "Fail to fscanf";
            return false;
        }
        return true;
#elif defined(FLARE_PLATFORM_OSX)
        // TODO(zhujiashun): get rchar, wchar, syscr, syscw, cancelled_write_bytes
        // in MacOS.
        memset(s, 0, sizeof(ProcIO));
        static pid_t pid = getpid();
        rusage_info_current rusage;
        if (proc_pid_rusage(pid, RUSAGE_INFO_CURRENT, (void **) &rusage) != 0) {
            FLARE_PLOG(WARNING) << "Fail to proc_pid_rusage";
            return false;
        }
        s->read_bytes = rusage.ri_diskio_bytesread;
        s->write_bytes = rusage.ri_diskio_byteswritten;
        return true;
#else
        return false;
#endif
    }

    class ProcIOReader {
    public:
        bool operator()(ProcIO *stat) const {
            return read_proc_io(stat);
        }

        template<typename T, size_t offset>
        static T get_field(void *) {
            return *(T *) ((char *) &CachedReader<ProcIO>::get_value(
                    ProcIOReader()) + offset);
        }
    };

#define VARIABLE_DEFINE_PROC_IO_FIELD(field)                                \
    status_gauge<VARIABLE_MEMBER_TYPE(&ProcIO::field)> g_##field(          \
        ProcIOReader::get_field<VARIABLE_MEMBER_TYPE(&ProcIO::field),       \
        offsetof(ProcIO, field)>, nullptr);

    // ==================================================
    // Refs:
    //   https://www.kernel.org/doc/Documentation/ABI/testing/procfs-diskstats
    //   https://www.kernel.org/doc/Documentation/iostats.txt
    //
    // The /proc/diskstats file displays the I/O statistics of block devices.
    // Each line contains the following 14 fields:
    struct DiskStat {
        long long major_number;
        long long minor_mumber;
        char device_name[64];

        // The total number of reads completed successfully.
        long long reads_completed; // wMB/s wKB/s

        // Reads and writes which are adjacent to each other may be merged for
        // efficiency.  Thus two 4K reads may become one 8K read before it is
        // ultimately handed to the disk, and so it will be counted (and queued)
        // as only one I/O.  This field lets you know how often this was done.
        long long reads_merged;     // rrqm/s

        // The total number of sectors read successfully.
        long long sectors_read;     // rsec/s

        // The total number of milliseconds spent by all reads (as
        // measured from __make_request() to end_that_request_last()).
        long long time_spent_reading_ms;

        // The total number of writes completed successfully.
        long long writes_completed; // rKB/s rMB/s

        // See description of reads_merged
        long long writes_merged;    // wrqm/s

        // The total number of sectors written successfully.
        long long sectors_written;  // wsec/s

        // The total number of milliseconds spent by all writes (as
        // measured from __make_request() to end_that_request_last()).
        long long time_spent_writing_ms;

        // The only field that should go to zero. Incremented as requests are
        // given to appropriate struct request_queue and decremented as they finish.
        long long io_in_progress;

        // This field increases so long as `io_in_progress' is nonzero.
        long long time_spent_io_ms;

        // This field is incremented at each I/O start, I/O completion, I/O
        // merge, or read of these stats by the number of I/Os in progress
        // `io_in_progress' times the number of milliseconds spent doing
        // I/O since the last update of this field.  This can provide an easy
        // measure of both I/O completion time and the backlog that may be
        // accumulating.
        long long weighted_time_spent_io_ms;
    };

    static bool read_disk_stat(DiskStat *s) {
#if defined(FLARE_PLATFORM_LINUX)
         flare::sequential_read_file file;
        auto status = file.open("/proc/diskstats");
        if (!status.is_ok()) {
            FLARE_PLOG_ONCE(WARNING) << "Fail to open /proc/diskstats";
            return false;
        }
        errno = 0;
        std::string content;
        file.read(&content);
        if (sscanf(content.c_str(), "%lld %lld %s %lld %lld %lld %lld %lld %lld %lld "
                   "%lld %lld %lld %lld",
                   &s->major_number,
                   &s->minor_mumber,
                   s->device_name,
                   &s->reads_completed,
                   &s->reads_merged,
                   &s->sectors_read,
                   &s->time_spent_reading_ms,
                   &s->writes_completed,
                   &s->writes_merged,
                   &s->sectors_written,
                   &s->time_spent_writing_ms,
                   &s->io_in_progress,
                   &s->time_spent_io_ms,
                   &s->weighted_time_spent_io_ms) != 14) {
            FLARE_PLOG(WARNING) << "Fail to fscanf";
            return false;
        }
        return true;
#elif defined(FLARE_PLATFORM_OSX)
        // TODO(zhujiashun)
        return false;
#else
        return false;
#endif
    }

    class DiskStatReader {
    public:
        bool operator()(DiskStat *stat) const {
            return read_disk_stat(stat);
        }

        template<typename T, size_t offset>
        static T get_field(void *) {
            return *(T *) ((char *) &CachedReader<DiskStat>::get_value(
                    DiskStatReader()) + offset);
        }
    };

#define VARIABLE_DEFINE_DISK_STAT_FIELD(field)                              \
    status_gauge<VARIABLE_MEMBER_TYPE(&DiskStat::field)> g_##field(        \
        DiskStatReader::get_field<VARIABLE_MEMBER_TYPE(&DiskStat::field),   \
        offsetof(DiskStat, field)>, nullptr);

// =====================================

    struct ReadSelfCmdline {
        std::string content;

        ReadSelfCmdline() {
            char buf[1024];
            const ssize_t nr = flare::read_command_line(buf, sizeof(buf), true);
            content.append(buf, nr);
        }
    };

    static void get_cmdline(std::ostream &os, void *) {
        os << flare::get_leaky_singleton<ReadSelfCmdline>()->content;
    }

    struct ReadVersion {
        std::string content;

        ReadVersion() {
            std::ostringstream oss;
            if (flare::read_command_output(oss, "uname -ap") != 0) {
                FLARE_LOG(ERROR) << "Fail to read kernel version";
                return;
            }
            content.append(oss.str());
        }
    };

    static void get_kernel_version(std::ostream &os, void *) {
        os << flare::get_leaky_singleton<ReadVersion>()->content;
    }

// ======================================

    static int64_t g_starting_time = flare::get_current_time_micros();

    static timeval get_uptime(void *) {
        int64_t uptime_us = flare::get_current_time_micros() - g_starting_time;
        timeval tm;
        tm.tv_sec = uptime_us / 1000000L;
        tm.tv_usec = uptime_us - tm.tv_sec * 1000000L;
        return tm;
    }

// ======================================

    class RUsageReader {
    public:
        bool operator()(rusage *stat) const {
            const int rc = getrusage(RUSAGE_SELF, stat);
            if (rc < 0) {
                FLARE_PLOG(WARNING) << "Fail to getrusage";
                return false;
            }
            return true;
        }

        template<typename T, size_t offset>
        static T get_field(void *) {
            return *(T *) ((char *) &CachedReader<rusage>::get_value(
                    RUsageReader()) + offset);
        }
    };

#define VARIABLE_DEFINE_RUSAGE_FIELD(field)                                 \
    status_gauge<VARIABLE_MEMBER_TYPE(&rusage::field)> g_##field(          \
        RUsageReader::get_field<VARIABLE_MEMBER_TYPE(&rusage::field),       \
        offsetof(rusage, field)>, nullptr);                                \

#define VARIABLE_DEFINE_RUSAGE_FIELD2(field, name)                          \
    status_gauge<VARIABLE_MEMBER_TYPE(&rusage::field)> g_##field(          \
        name,                                                           \
        RUsageReader::get_field<VARIABLE_MEMBER_TYPE(&rusage::field),       \
        offsetof(rusage, field)>, nullptr);                                \

// ======================================

    VARIABLE_DEFINE_PROC_STAT_FIELD2(pid, "pid");
    VARIABLE_DEFINE_PROC_STAT_FIELD2(ppid, "ppid");
    VARIABLE_DEFINE_PROC_STAT_FIELD2(pgrp, "pgrp");

    static void get_username(std::ostream &os, void *) {
        char buf[32];
        if (getlogin_r(buf, sizeof(buf)) == 0) {
            buf[sizeof(buf) - 1] = '\0';
            os << buf;
        } else {
            os << "unknown (" << flare_error() << ')';
        }
    }

    status_gauge<std::string> g_username(
            "process_username", get_username, nullptr);

    VARIABLE_DEFINE_PROC_STAT_FIELD(minflt);
    per_second<status_gauge<unsigned long>> g_minflt_second(
            "process_faults_minor_second", &g_minflt);
    VARIABLE_DEFINE_PROC_STAT_FIELD2(majflt, "process_faults_major");

    VARIABLE_DEFINE_PROC_STAT_FIELD2(priority, "process_priority");
    VARIABLE_DEFINE_PROC_STAT_FIELD2(nice, "process_nice");

    VARIABLE_DEFINE_PROC_STAT_FIELD2(num_threads, "process_thread_count");
    status_gauge<int> g_fd_num("process_fd_count", print_fd_count, nullptr);

    VARIABLE_DEFINE_PROC_MEMORY_FIELD(size, "process_memory_virtual");
    VARIABLE_DEFINE_PROC_MEMORY_FIELD(resident, "process_memory_resident");
    VARIABLE_DEFINE_PROC_MEMORY_FIELD(share, "process_memory_shared");
    VARIABLE_DEFINE_PROC_MEMORY_FIELD(trs, "process_memory_text");
    VARIABLE_DEFINE_PROC_MEMORY_FIELD(drs, "process_memory_data_and_stack");

    VARIABLE_DEFINE_LOAD_AVERAGE_FIELD(loadavg_1m, "system_loadavg_1m");
    VARIABLE_DEFINE_LOAD_AVERAGE_FIELD(loadavg_5m, "system_loadavg_5m");
    VARIABLE_DEFINE_LOAD_AVERAGE_FIELD(loadavg_15m, "system_loadavg_15m");

    VARIABLE_DEFINE_PROC_IO_FIELD(rchar);
    VARIABLE_DEFINE_PROC_IO_FIELD(wchar);
    per_second<status_gauge<size_t>> g_io_read_second(
            "process_io_read_bytes_second", &g_rchar);
    per_second<status_gauge<size_t>> g_io_write_second(
            "process_io_write_bytes_second", &g_wchar);

    VARIABLE_DEFINE_PROC_IO_FIELD(syscr);
    VARIABLE_DEFINE_PROC_IO_FIELD(syscw);
    per_second<status_gauge<size_t>> g_io_num_reads_second(
            "process_io_read_second", &g_syscr);
    per_second<status_gauge<size_t>> g_io_num_writes_second(
            "process_io_write_second", &g_syscw);

    VARIABLE_DEFINE_PROC_IO_FIELD(read_bytes);
    VARIABLE_DEFINE_PROC_IO_FIELD(write_bytes);
    per_second<status_gauge<size_t>> g_disk_read_second(
            "process_disk_read_bytes_second", &g_read_bytes);
    per_second<status_gauge<size_t>> g_disk_write_second(
            "process_disk_write_bytes_second", &g_write_bytes);

    VARIABLE_DEFINE_RUSAGE_FIELD(ru_utime);
    VARIABLE_DEFINE_RUSAGE_FIELD(ru_stime);
    status_gauge<timeval> g_uptime("process_uptime", get_uptime, nullptr);

    static int get_core_num(void *) {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }

    status_gauge<int> g_core_num("system_core_count", get_core_num, nullptr);

    struct TimePercent {
        int64_t time_us;
        int64_t real_time_us;

        void operator-=(const TimePercent &rhs) {
            time_us -= rhs.time_us;
            real_time_us -= rhs.real_time_us;
        }

        void operator+=(const TimePercent &rhs) {
            time_us += rhs.time_us;
            real_time_us += rhs.real_time_us;
        }
    };

    inline std::ostream &operator<<(std::ostream &os, const TimePercent &tp) {
        if (tp.real_time_us <= 0) {
            return os << "0";
        } else {
            return os << std::fixed << std::setprecision(3)
                      << (double) tp.time_us / tp.real_time_us;
        }
    }

    static TimePercent get_cputime_percent(void *) {
        TimePercent tp = {(flare::time_point::from_timeval(g_ru_stime.get_value()) +
                           flare::duration::from_timeval(g_ru_utime.get_value())).to_unix_micros(),
                          flare::time_point::from_timeval(g_uptime.get_value()).to_unix_micros()};
        return tp;
    }

    status_gauge<TimePercent> g_cputime_percent(get_cputime_percent, nullptr);
    window<status_gauge<TimePercent>, SERIES_IN_SECOND> g_cputime_percent_second(
            "process_cpu_usage", &g_cputime_percent, FLAGS_variable_dump_interval);

    static TimePercent get_stime_percent(void *) {
        TimePercent tp = {flare::time_point::from_timeval(g_ru_stime.get_value()).to_unix_micros(),
                          flare::time_point::from_timeval(g_uptime.get_value()).to_unix_micros()};
        return tp;
    }

    status_gauge<TimePercent> g_stime_percent(get_stime_percent, nullptr);
    window<status_gauge<TimePercent>, SERIES_IN_SECOND> g_stime_percent_second(
            "process_cpu_usage_system", &g_stime_percent, FLAGS_variable_dump_interval);

    static TimePercent get_utime_percent(void *) {
        TimePercent tp = {flare::time_point::from_timeval(g_ru_utime.get_value()).to_unix_micros(),
                          flare::time_point::from_timeval(g_uptime.get_value()).to_unix_micros()};
        return tp;
    }

    status_gauge<TimePercent> g_utime_percent(get_utime_percent, nullptr);
    window<status_gauge<TimePercent>, SERIES_IN_SECOND> g_utime_percent_second(
            "process_cpu_usage_user", &g_utime_percent, FLAGS_variable_dump_interval);

    // According to http://man7.org/linux/man-pages/man2/getrusage.2.html
    // Unsupported fields in linux:
    //   ru_ixrss
    //   ru_idrss
    //   ru_isrss
    //   ru_nswap
    //   ru_nsignals
    VARIABLE_DEFINE_RUSAGE_FIELD(ru_inblock);
    VARIABLE_DEFINE_RUSAGE_FIELD(ru_oublock);
    VARIABLE_DEFINE_RUSAGE_FIELD(ru_nvcsw);
    VARIABLE_DEFINE_RUSAGE_FIELD(ru_nivcsw);
    per_second<status_gauge<long>> g_ru_inblock_second(
            "process_inblocks_second", &g_ru_inblock);
    per_second<status_gauge<long>> g_ru_oublock_second(
            "process_outblocks_second", &g_ru_oublock);
    per_second<status_gauge<long>> cs_vol_second(
            "process_context_switches_voluntary_second", &g_ru_nvcsw);
    per_second<status_gauge<long>> cs_invol_second(
            "process_context_switches_involuntary_second", &g_ru_nivcsw);

    status_gauge<std::string> g_cmdline("process_cmdline", get_cmdline, nullptr);
    status_gauge<std::string> g_kernel_version(
            "kernel_version", get_kernel_version, nullptr);

    static std::string *s_gcc_version = nullptr;
    pthread_once_t g_gen_gcc_version_once = PTHREAD_ONCE_INIT;

    void gen_gcc_version() {

#if defined(__GNUC__)
        const int gcc_major = __GNUC__;
#else
        const int gcc_major = -1;
#endif

#if defined(__GNUC_MINOR__)
        const int gcc_minor = __GNUC_MINOR__;
#else
        const int gcc_minor = -1;
#endif

#if defined(__GNUC_PATCHLEVEL__)
        const int gcc_patchlevel = __GNUC_PATCHLEVEL__;
#else
        const int gcc_patchlevel = -1;
#endif

        s_gcc_version = new std::string;

        if (gcc_major == -1) {
            *s_gcc_version = "unknown";
            return;
        }
        std::ostringstream oss;
        oss << gcc_major;
        if (gcc_minor == -1) {
            return;
        }
        oss << '.' << gcc_minor;

        if (gcc_patchlevel == -1) {
            return;
        }
        oss << '.' << gcc_patchlevel;

        *s_gcc_version = oss.str();

    }

    void get_gcc_version(std::ostream &os, void *) {
        pthread_once(&g_gen_gcc_version_once, gen_gcc_version);
        os << *s_gcc_version;
    }

// =============================================
    status_gauge<std::string> g_gcc_version("gcc_version", get_gcc_version, nullptr);

    void get_work_dir(std::ostream &os, void *) {
        std::error_code ec;
        flare::file_path curr = flare::current_path(ec);
        FLARE_LOG_IF(WARNING, ec) << "Fail to GetCurrentDirectory";
        os << curr.c_str();
    }

    status_gauge<std::string> g_work_dir("process_work_dir", get_work_dir, nullptr);

#undef VARIABLE_MEMBER_TYPE
#undef VARIABLE_DEFINE_PROC_STAT_FIELD
#undef VARIABLE_DEFINE_PROC_STAT_FIELD2
#undef VARIABLE_DEFINE_PROC_MEMORY_FIELD
#undef VARIABLE_DEFINE_RUSAGE_FIELD
#undef VARIABLE_DEFINE_RUSAGE_FIELD2

}  // namespace flare

// In the same scope where timeval is defined. Required by clang.
inline std::ostream &operator<<(std::ostream &os, const timeval &tm) {
    return os << tm.tv_sec << '.' << std::setw(6) << std::setfill('0') << tm.tv_usec;
}

#endif  // UNIT_TEST