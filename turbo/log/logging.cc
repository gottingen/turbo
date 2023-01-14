
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/


#include <algorithm>
#include <cassert>
#include "turbo/files/filesystem.h"
#include <iomanip>
#include <string>
#include <unistd.h>  // For _exit.
#include <climits>
#include <sys/types.h>
#include <mutex>
#include <sys/stat.h>
#include <sys/utsname.h>  // For uname.
#include <ctime>
#include <fcntl.h>
#include <cstdio>
#include <iostream>
#include <cstdarg>
#include <cstdlib>
#include <pwd.h>
#include <syslog.h>
#include <vector>
#include <cerrno>                   // for errno
#include <sstream>
#include <dirent.h> // for automatic removal of old logs
#include "turbo/log/config.h"        // to get the program name
#include "turbo/log/logging.h"
#include "turbo/log/raw_logging.h"
#include "turbo/log/init.h"
#include "turbo/log/utility.h"
#include "turbo/thread/thread.h"
#include "turbo/system/sysinfo.h"

#ifdef TURBO_PLATFORM_LINUX
#include <signal.h>
#endif
#ifdef HAVE_STACKTRACE

# include "turbo/debugging/stacktrace.h"

#endif

#ifdef __ANDROID__
#include <android/log.h>
#endif

using std::string;
using std::vector;
using std::setw;
using std::setfill;
using std::hex;
using std::dec;
using std::min;
using std::ostream;
using std::ostringstream;

using std::FILE;
using std::fwrite;
using std::fclose;
using std::fflush;
using std::fprintf;
using std::perror;

#ifdef __QNX__
using std::fdopen;
#endif

#ifdef _WIN32
#define fdopen _fdopen
#endif

// There is no thread annotation support.
#define EXCLUSIVE_LOCKS_REQUIRED(mu)

// TODO(hamaji): consider windows
#define PATH_SEPARATOR '/'

#ifdef TURBO_PLATFORM_LINUX
DECLARE_bool(drop_log_memory);
#endif

// Returns true iff terminal supports using colors in output.
static bool TerminalSupportsColor() {
    bool term_supports_color = false;
    // On non-Windows platforms, we rely on the TERM variable.
    const char *const term = getenv("TERM");
    if (term != nullptr && term[0] != '\0') {
        term_supports_color =
                !strcmp(term, "xterm") ||
                !strcmp(term, "xterm-color") ||
                !strcmp(term, "xterm-256color") ||
                !strcmp(term, "screen-256color") ||
                !strcmp(term, "konsole") ||
                !strcmp(term, "konsole-16color") ||
                !strcmp(term, "konsole-256color") ||
                !strcmp(term, "screen") ||
                !strcmp(term, "linux") ||
                !strcmp(term, "cygwin");
    }
    return term_supports_color;
}

namespace turbo::log {

    enum log_color {
        COLOR_DEFAULT,
        COLOR_RED,
        COLOR_GREEN,
        COLOR_YELLOW
    };

    static log_color SeverityToColor(log_severity severity) {
        assert(severity >= 0 && severity < NUM_SEVERITIES);
        log_color color = COLOR_DEFAULT;
        switch (severity) {
            case TURBO_INFO:
                color = COLOR_DEFAULT;
                break;
            case TURBO_WARNING:
                color = COLOR_YELLOW;
                break;
            case TURBO_ERROR:
            case TURBO_FATAL:
                color = COLOR_RED;
                break;
            default:
                // should never get here.
                assert(false);
        }
        return color;
    }


    // Returns the ANSI color code for the given color.
    static const char *GetAnsiColorCode(log_color color) {
        switch (color) {
            case COLOR_RED:
                return "1";
            case COLOR_GREEN:
                return "2";
            case COLOR_YELLOW:
                return "3";
            case COLOR_DEFAULT:
                return "";
        };
        return nullptr; // stop warning about return type.
    }


    // Safely get turbo_max_log_size, overriding to 1 if it somehow gets defined as 0
    static int32_t MaxLogSize() {
        return (FLAGS_turbo_max_log_size > 0 && FLAGS_turbo_max_log_size < 4096 ? FLAGS_turbo_max_log_size : 1);
    }

    // An arbitrary limit on the length of a single log message.  This
    // is so that streaming can be done more efficiently.
    const size_t log_message::kMaxLogMessageLen = 30000;

    struct log_message::log_message_data {
        log_message_data();

        int preserved_errno_;      // preserved errno
        // Buffer space; contains complete message text.
        char message_text_[log_message::kMaxLogMessageLen + 1];
        log_stream stream_;
        char severity_;      // What level is this log_message logged at?
        int line_;                 // line number where logging call is.
        void (log_message::*send_method_)();  // Call this in destructor to send
        union {  // At most one of these is used: union to keep the size low.
            log_sink *sink_;             // nullptr or sink to send message to
            std::vector<std::string> *outvec_; // nullptr or vector to push message onto
            std::string *message_;             // nullptr or string to write message into
        };
        time_t timestamp_;            // Time of creation of log_message
        struct ::tm tm_time_;         // Time of creation of log_message
        int32_t usecs_;                   // Time of creation of log_message - microseconds part
        size_t num_prefix_chars_;     // # of chars of prefix in this message
        size_t num_chars_to_log_;     // # of chars of msg to send to log
        size_t num_chars_to_syslog_;  // # of chars of msg to send to syslog
        const char *basename_;        // basename of file that called TURBO_LOG
        const char *fullname_;        // fullname of file that called TURBO_LOG
        bool has_been_flushed_;       // false => data has not been flushed
        bool first_fatal_;            // true => this was first fatal msg

    private:
        log_message_data(const log_message_data &);

        void operator=(const log_message_data &);
    };

    // A mutex that allows only one thread to log at a time, to keep things from
    // getting jumbled.  Some other very uncommon logging operations (like
    // changing the destination file for log messages of a given severity) also
    // lock this mutex.  Please be sure that anybody who might possibly need to
    // lock it does so.
    static std::mutex log_mutex;

    // Number of messages sent at each severity.  Under log_mutex.
    int64_t log_message::num_messages_[NUM_SEVERITIES] = {0, 0, 0, 0, 0, 0};

    // Globally disable log writing (if disk is full)
    static bool stop_writing = false;

    const char *const log_severity_names[NUM_SEVERITIES] = {
            "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
    };

    const char *get_log_severity_name(log_severity severity) {
        return log_severity_names[severity];
    }

    static bool SendEmailInternal(const char *dest, const char *subject,
                                  const char *body, bool use_logging);

    base::inner_logger::~inner_logger() {
    }


    namespace {

        // Encapsulates all file-system related state
        class log_file_object : public base::inner_logger {
        public:
            log_file_object(log_severity severity, const char *base_filename);

            ~log_file_object();

            virtual void write(bool force_flush, // Should we force a flush here?
                               time_t timestamp,  // Timestamp for this entry
                               const char *message,
                               int message_len);

            // Configuration options
            void set_basename(const char *basename);

            void set_extension(const char *ext);

            void set_symlink_basename(const char *symlink_basename);

            // Normal flushing routine
            virtual void flush();

            // It is the actual file length for the system loggers,
            // i.e., INFO, ERROR, etc.
            virtual uint32_t log_size() {
                std::unique_lock<std::mutex> l(lock_);
                return file_length_;
            }

            // Internal flush routine.  Exposed so that flush_log_files_unsafe()
            // can avoid grabbing a lock.  Usually Flush() calls it after
            // acquiring lock_.
            void flush_unlocked();

        private:
            static const uint32_t kRolloverAttemptFrequency = 0x20;

            std::mutex lock_;
            bool base_filename_selected_;
            string base_filename_;
            string symlink_basename_;
            string filename_extension_;     // option users can specify (eg to add port#)
            FILE *file_;
            log_severity severity_;
            uint32_t bytes_since_flush_;
            uint32_t dropped_mem_length_;
            uint32_t file_length_;
            unsigned int rollover_attempt_;
            turbo::time_point next_flush_time_;         // cycle count at which to flush log
            turbo::time_point start_time_;

            // Actually create a logfile using the value of base_filename_ and the
            // optional argument time_pid_string
            // REQUIRES: lock_ is held
            bool create_logfile(const string &time_pid_string);
        };

        // Encapsulate all log cleaner related states
        class log_cleaner {
        public:
            log_cleaner();

            virtual ~log_cleaner() {}

            void enable(int overdue_days);

            void disable();

            void run(bool base_filename_selected,
                     const string &base_filename,
                     const string &filename_extension) const;

            inline bool enabled() const { return enabled_; }

        private:
            vector<string> get_overdue_log_names(string log_directory,
                                                 int days,
                                                 const string &base_filename,
                                                 const string &filename_extension) const;

            bool is_log_from_current_project(const string &filepath,
                                             const string &base_filename,
                                             const string &filename_extension) const;

            bool is_log_last_modified_over(const string &filepath, int days) const;

            bool enabled_;
            int overdue_days_;
            char dir_delim_;  // filepath delimiter ('/' or '\\')
        };

        log_cleaner g_log_cleaner;

    }  // namespace

    class log_destination {
    public:
        friend class log_message;

        friend void reprint_fatal_message();

        friend base::inner_logger *base::get_logger(log_severity);

        friend void base::set_logger(log_severity, base::inner_logger *);

        // These methods are just forwarded to by their global versions.
        static void set_log_destination(log_severity severity,
                                        const char *base_filename);

        static void set_log_symlink(log_severity severity,
                                    const char *symlink_basename);

        static void add_log_sink(log_sink *destination);

        static void remove_log_sink(log_sink *destination);

        static void set_log_filename_extension(const char *filename_extension);

        static void set_stderr_logging(log_severity min_severity);

        static void set_email_logging(log_severity min_severity, const char *addresses);

        static void log_to_stderr();

        // Flush all log files that are at least at the given severity level
        static void flush_log_files(int min_severity);

        static void flush_log_files_unsafe(int min_severity);

        // we set the maximum size of our packet to be 1400, the logic being
        // to prevent fragmentation.
        // Really this number is arbitrary.
        static const int kNetworkBytes = 1400;

        static const string &hostname();

        static const bool &terminal_supports_color() {
            return terminal_supports_color_;
        }

        static void delete_log_destinations();

    private:
        log_destination(log_severity severity, const char *base_filename);

        ~log_destination();

        // Take a log message of a particular severity and log it to stderr
        // iff it's of a high enough severity to deserve it.
        static void maybe_log_to_stderr(log_severity severity, const char *message,
                                        size_t message_len, size_t prefix_len);

        // Take a log message of a particular severity and log it to email
        // iff it's of a high enough severity to deserve it.
        static void maybe_log_to_email(log_severity severity, const char *message,
                                       size_t len);

        // Take a log message of a particular severity and log it to a file
        // iff the base filename is not "" (which means "don't log to me")
        static void maybe_log_to_logfile(log_severity severity,
                                         time_t timestamp,
                                         const char *message, size_t len);

        // Take a log message of a particular severity and log it to the file
        // for that severity and also for all files with severity less than
        // this severity.
        static void log_to_all_logfiles(log_severity severity,
                                        time_t timestamp,
                                        const char *message, size_t len);

        // Send logging info to all registered sinks.
        static void log_to_sinks(log_severity severity,
                                 const char *full_filename,
                                 const char *base_filename,
                                 int line,
                                 const struct ::tm *tm_time,
                                 const char *message,
                                 size_t message_len,
                                 int32_t usecs);

        // Wait for all registered sinks via WaitTillSent
        // including the optional one in "data".
        static void wait_for_sinks(log_message::log_message_data *data);

        static log_destination *get_log_destination(log_severity severity);

        log_file_object fileobject_;
        base::inner_logger *logger_;      // Either &fileobject_, or wrapper around it

        static log_destination *log_destinations_[NUM_SEVERITIES];
        static log_severity email_logging_severity_;
        static string addresses_;
        static string hostname_;
        static bool terminal_supports_color_;

        // arbitrary global logging destinations.
        static vector<log_sink *> *sinks_;

        // Protects the vector sinks_,
        // but not the log_sink objects its elements reference.
        static std::mutex sink_mutex_;

        // Disallow
        log_destination(const log_destination &);

        log_destination &operator=(const log_destination &);
    };

// Errors do not get logged to email by default.
    log_severity log_destination::email_logging_severity_ = 99999;

    string log_destination::addresses_;
    string log_destination::hostname_;

    vector<log_sink *> *log_destination::sinks_ = nullptr;
    std::mutex log_destination::sink_mutex_;
    bool log_destination::terminal_supports_color_ = TerminalSupportsColor();

/* static */
    const string &log_destination::hostname() {
        if (hostname_.empty()) {
            hostname_ = turbo::sysinfo::get_host_name();
            if (hostname_.empty()) {
                hostname_ = "(unknown)";
            }
        }
        return hostname_;
    }

    log_destination::log_destination(log_severity severity,
                                     const char *base_filename)
            : fileobject_(severity, base_filename),
              logger_(&fileobject_) {
    }

    log_destination::~log_destination() {
        if (logger_ && logger_ != &fileobject_) {
            // Delete user-specified logger set via set_logger().
            delete logger_;
        }
    }

    inline void log_destination::flush_log_files_unsafe(int min_severity) {
        // assume we have the log_mutex or we simply don't care
        // about it
        for (int i = min_severity; i < NUM_SEVERITIES; i++) {
            log_destination *log = log_destinations_[i];
            if (log != nullptr) {
                // Flush the base fileobject_ logger directly instead of going
                // through any wrappers to reduce chance of deadlock.
                log->fileobject_.flush_unlocked();
            }
        }
    }

    inline void log_destination::flush_log_files(int min_severity) {
        // Prevent any subtle race conditions by wrapping a mutex lock around
        // all this stuff.
        std::unique_lock<std::mutex> l(log_mutex);
        for (int i = min_severity; i < NUM_SEVERITIES; i++) {
            log_destination *log = get_log_destination(i);
            if (log != nullptr) {
                log->logger_->flush();
            }
        }
    }

    inline void log_destination::set_log_destination(log_severity severity,
                                                     const char *base_filename) {
        assert(severity >= 0 && severity < NUM_SEVERITIES);
        // Prevent any subtle race conditions by wrapping a mutex lock around
        // all this stuff.
        std::unique_lock<std::mutex> l(log_mutex);
        get_log_destination(severity)->fileobject_.set_basename(base_filename);
    }

    inline void log_destination::set_log_symlink(log_severity severity,
                                                 const char *symlink_basename) {
        TURBO_CHECK_GE(severity, 0);
        TURBO_CHECK_LT(severity, NUM_SEVERITIES);
        std::unique_lock<std::mutex> l(log_mutex);
        get_log_destination(severity)->fileobject_.set_symlink_basename(symlink_basename);
    }

    inline void log_destination::add_log_sink(log_sink *destination) {
        // Prevent any subtle race conditions by wrapping a mutex lock around
        // all this stuff.
        std::unique_lock<std::mutex> l(sink_mutex_);
        if (!sinks_) sinks_ = new vector<log_sink *>;
        sinks_->push_back(destination);
    }

    inline void log_destination::remove_log_sink(log_sink *destination) {
        // Prevent any subtle race conditions by wrapping a mutex lock around
        // all this stuff.
        std::unique_lock<std::mutex> l(sink_mutex_);
        // This doesn't keep the sinks in order, but who cares?
        if (sinks_) {
            for (int i = sinks_->size() - 1; i >= 0; i--) {
                if ((*sinks_)[i] == destination) {
                    (*sinks_)[i] = (*sinks_)[sinks_->size() - 1];
                    sinks_->pop_back();
                    break;
                }
            }
        }
    }

    inline void log_destination::set_log_filename_extension(const char *ext) {
        // Prevent any subtle race conditions by wrapping a mutex lock around
        // all this stuff.
        std::unique_lock<std::mutex> l(log_mutex);
        for (int severity = 0; severity < NUM_SEVERITIES; ++severity) {
            get_log_destination(severity)->fileobject_.set_extension(ext);
        }
    }

    inline void log_destination::set_stderr_logging(log_severity min_severity) {
        assert(min_severity >= 0 && min_severity < NUM_SEVERITIES);
        // Prevent any subtle race conditions by wrapping a mutex lock around
        // all this stuff.
        std::unique_lock<std::mutex> l(log_mutex);
        FLAGS_turbo_stderrthreshold = min_severity;
    }

    inline void log_destination::log_to_stderr() {
        // *Don't* put this stuff in a mutex lock, since set_stderr_logging &
        // set_log_destination already do the locking!
        set_stderr_logging(0);            // thus everything is "also" logged to stderr
        for (int i = 0; i < NUM_SEVERITIES; ++i) {
            set_log_destination(i, "");     // "" turns off logging to a logfile
        }
    }

    inline void log_destination::set_email_logging(log_severity min_severity,
                                                   const char *addresses) {
        assert(min_severity >= 0 && min_severity < NUM_SEVERITIES);
        // Prevent any subtle race conditions by wrapping a mutex lock around
        // all this stuff.
        std::unique_lock<std::mutex> l(log_mutex);
        log_destination::email_logging_severity_ = min_severity;
        log_destination::addresses_ = addresses;
    }

    static void colored_write_to_stderr(log_severity severity,
                                        const char *message, size_t len) {
        const log_color color =
                (log_destination::terminal_supports_color() && FLAGS_turbo_colorlogtostderr) ?
                SeverityToColor(severity) : COLOR_DEFAULT;

        // Avoid using cerr from this module since we may get called during
        // exit code, and cerr may be partially or fully destroyed by then.
        if (COLOR_DEFAULT == color) {
            fwrite(message, len, 1, stderr);
            return;
        }
        fprintf(stderr, "\033[0;3%sm", GetAnsiColorCode(color));
        fwrite(message, len, 1, stderr);
        fprintf(stderr, "\033[m");  // Resets the terminal to default.
    }

    static void write_to_stderr(const char *message, size_t len) {
        // Avoid using cerr from this module since we may get called during
        // exit code, and cerr may be partially or fully destroyed by then.
        fwrite(message, len, 1, stderr);
    }

    inline void log_destination::maybe_log_to_stderr(log_severity severity,
                                                     const char *message, size_t message_len, size_t prefix_len) {
        if ((severity >= FLAGS_turbo_stderrthreshold) || FLAGS_turbo_also_logtostderr) {
            colored_write_to_stderr(severity, message, message_len);
        }
    }


    inline void log_destination::maybe_log_to_email(log_severity severity,
                                                    const char *message, size_t len) {
        if (severity >= email_logging_severity_ ||
            severity >= FLAGS_turbo_log_email_level) {
            std::string to(FLAGS_turbo_also_log_to_email);
            if (!addresses_.empty()) {
                if (!to.empty()) {
                    to += ",";
                }
                to += addresses_;
            }
            const string subject(string("[TURBO_LOG] ") + log_severity_names[severity] + ": " +
                                 log_internal::program_invocation_short_name());
            string body(hostname());
            body += "\n\n";
            body.append(message, len);

            // should NOT use SendEmail().  The caller of this function holds the
            // log_mutex and SendEmail() calls TURBO_LOG/TURBO_VLOG which will block trying to
            // acquire the log_mutex object.  Use SendEmailInternal() and set
            // use_logging to false.
            SendEmailInternal(to.c_str(), subject.c_str(), body.c_str(), false);
        }
    }


    inline void log_destination::maybe_log_to_logfile(log_severity severity,
                                                      time_t timestamp,
                                                      const char *message,
                                                      size_t len) {
        const bool should_flush = severity > FLAGS_turbo_logbuflevel;
        log_destination *destination = get_log_destination(severity);
        destination->logger_->write(should_flush, timestamp, message, len);
    }

    inline void log_destination::log_to_all_logfiles(log_severity severity,
                                                     time_t timestamp,
                                                     const char *message,
                                                     size_t len) {

        if (FLAGS_turbo_logtostderr) {           // global flag: never log to file
            colored_write_to_stderr(severity, message, len);
        } else {
            for (int i = severity; i >= 0; --i)
                log_destination::maybe_log_to_logfile(i, timestamp, message, len);
        }
    }

    inline void log_destination::log_to_sinks(log_severity severity,
                                              const char *full_filename,
                                              const char *base_filename,
                                              int line,
                                              const struct ::tm *tm_time,
                                              const char *message,
                                              size_t message_len,
                                              int32_t usecs) {
        std::unique_lock<std::mutex> l(sink_mutex_);
        if (sinks_) {
            for (int i = sinks_->size() - 1; i >= 0; i--) {
                (*sinks_)[i]->send(severity, full_filename, base_filename,
                                   line, tm_time, message, message_len, usecs);
            }
        }
    }

    inline void log_destination::wait_for_sinks(log_message::log_message_data *data) {
        std::unique_lock<std::mutex> l(sink_mutex_);
        if (sinks_) {
            for (int i = sinks_->size() - 1; i >= 0; i--) {
                (*sinks_)[i]->WaitTillSent();
            }
        }
        const bool send_to_sink =
                (data->send_method_ == &log_message::send_to_sink) ||
                (data->send_method_ == &log_message::send_to_sink_and_log);
        if (send_to_sink && data->sink_ != nullptr) {
            data->sink_->WaitTillSent();
        }
    }

    log_destination *log_destination::log_destinations_[NUM_SEVERITIES];

    inline log_destination *log_destination::get_log_destination(log_severity severity) {
        assert(severity >= 0 && severity < NUM_SEVERITIES);
        if (!log_destinations_[severity]) {
            log_destinations_[severity] = new log_destination(severity, nullptr);
        }
        return log_destinations_[severity];
    }

    void log_destination::delete_log_destinations() {
        for (int severity = 0; severity < NUM_SEVERITIES; ++severity) {
            delete log_destinations_[severity];
            log_destinations_[severity] = nullptr;
        }
        std::unique_lock<std::mutex> l(sink_mutex_);
        delete sinks_;
        sinks_ = nullptr;
    }

    namespace {

        std::string g_application_fingerprint;

    } // namespace

    void SetApplicationFingerprint(const std::string &fingerprint) {
        g_application_fingerprint = fingerprint;
    }

    namespace {

        string pretty_duration(const turbo::duration &d) {
            return fmt::format("{:02}:{:02}:{:02}", d.to_int64_hours(), d.to_int64_minutes(), d.to_int64_seconds());
        }


        log_file_object::log_file_object(log_severity severity,
                                         const char *base_filename)
                : base_filename_selected_(base_filename != nullptr),
                  base_filename_((base_filename != nullptr) ? base_filename : ""),
                  symlink_basename_(log_internal::program_invocation_short_name()),
                  filename_extension_(),
                  file_(nullptr),
                  severity_(severity),
                  bytes_since_flush_(0),
                  dropped_mem_length_(0),
                  file_length_(0),
                  rollover_attempt_(kRolloverAttemptFrequency - 1),
                  next_flush_time_(),
                  start_time_(turbo::time_now()) {
            assert(severity >= 0);
            assert(severity < NUM_SEVERITIES);
        }

        log_file_object::~log_file_object() {
            std::unique_lock<std::mutex> l(lock_);
            if (file_ != nullptr) {
                fclose(file_);
                file_ = nullptr;
            }
        }

        void log_file_object::set_basename(const char *basename) {
            std::unique_lock<std::mutex> l(lock_);
            base_filename_selected_ = true;
            if (base_filename_ != basename) {
                // Get rid of old log file since we are changing names
                if (file_ != nullptr) {
                    fclose(file_);
                    file_ = nullptr;
                    rollover_attempt_ = kRolloverAttemptFrequency - 1;
                }
                base_filename_ = basename;
            }
        }

        void log_file_object::set_extension(const char *ext) {
            std::unique_lock<std::mutex> l(lock_);
            if (filename_extension_ != ext) {
                // Get rid of old log file since we are changing names
                if (file_ != nullptr) {
                    fclose(file_);
                    file_ = nullptr;
                    rollover_attempt_ = kRolloverAttemptFrequency - 1;
                }
                filename_extension_ = ext;
            }
        }

        void log_file_object::set_symlink_basename(const char *symlink_basename) {
            std::unique_lock<std::mutex> l(lock_);
            symlink_basename_ = symlink_basename;
        }

        void log_file_object::flush() {
            std::unique_lock<std::mutex> l(lock_);
            flush_unlocked();
        }

        void log_file_object::flush_unlocked() {
            if (file_ != nullptr) {
                fflush(file_);
                bytes_since_flush_ = 0;
            }
            // Figure out when we are due for another flush.
            const turbo::duration next = turbo::duration::seconds(FLAGS_turbo_logbufsecs);  // in usec
            next_flush_time_ = turbo::time_now() + next;
        }

        bool log_file_object::create_logfile(const string &time_pid_string) {
            string string_filename = base_filename_;
            if (FLAGS_turbo_timestamp_in_logfile_name) {
                string_filename += time_pid_string;
            }
            string_filename += filename_extension_;
            const char *filename = string_filename.c_str();
            //only write to files, create if non-existant.
            int flags = O_WRONLY | O_CREAT;
            if (FLAGS_turbo_timestamp_in_logfile_name) {
                //demand that the file is unique for our timestamp (fail if it exists).
                flags = flags | O_EXCL;
            }
            int fd = open(filename, flags, FLAGS_turbo_logfile_mode);
            if (fd == -1) return false;
#ifdef HAVE_FCNTL
            // Mark the file close-on-exec. We don't really care if this fails
      fcntl(fd, F_SETFD, FD_CLOEXEC);

      // Mark the file as exclusive write access to avoid two clients logging to the
      // same file. This applies particularly when !FLAGS_turbo_timestamp_in_logfile_name
      // (otherwise open would fail because the O_EXCL flag on similar filename).
      // locks are released on unlock or close() automatically, only after log is
      // released.
      // This will work after a fork as it is not inherited (not stored in the fd).
      // Lock will not be lost because the file is opened with exclusive lock (write)
      // and we will never read from it inside the process.
      // TODO windows implementation of this (as flock is not available on mingw).
      static struct flock w_lock;

      w_lock.l_type = F_WRLCK;
      w_lock.l_start = 0;
      w_lock.l_whence = SEEK_SET;
      w_lock.l_len = 0;

      int wlock_ret = fcntl(fd, F_SETLK, &w_lock);
      if (wlock_ret == -1) {
          close(fd); //as we are failing already, do not check errors here
          return false;
      }
#endif

            //fdopen in append mode so if the file exists it will fseek to the end
            file_ = fdopen(fd, "a");  // Make a FILE*.
            if (file_ == nullptr) {  // Man, we're screwed!
                close(fd);
                if (FLAGS_turbo_timestamp_in_logfile_name) {
                    unlink(filename);  // Erase the half-baked evidence: an unusable log file, only if we just created it.
                }
                return false;
            }

            // We try to create a symlink called <program_name>.<severity>,
            // which is easier to use.  (Every time we create a new logfile,
            // we destroy the old symlink and create a new one, so it always
            // points to the latest logfile.)  If it fails, we're sad but it's
            // no error.
            if (!symlink_basename_.empty()) {
                // take directory from filename
                const char *slash = strrchr(filename, PATH_SEPARATOR);
                const string linkname =
                        symlink_basename_ + '.' + log_severity_names[severity_];
                string linkpath;
                if (slash) linkpath = string(filename, slash - filename + 1);  // get dirname
                linkpath += linkname;
                unlink(linkpath.c_str());                    // delete old one if it exists

                // We must have unistd.h.
                // Make the symlink be relative (in the same dir) so that if the
                // entire log directory gets relocated the link is still valid.
                const char *linkdest = slash ? (slash + 1) : filename;
                if (symlink(linkdest, linkpath.c_str()) != 0) {
                    // silently ignore failures
                }

                // Make an additional link to the log file in a place specified by
                // FLAGS_turbo_log_link, if indicated
                if (!FLAGS_turbo_log_link.empty()) {
                    linkpath = FLAGS_turbo_log_link + "/" + linkname;
                    unlink(linkpath.c_str());                  // delete old one if it exists
                    if (symlink(filename, linkpath.c_str()) != 0) {
                        // silently ignore failures
                    }
                }
            }

            return true;  // Everything worked
        }

        void log_file_object::write(bool force_flush,
                                    time_t timestamp,
                                    const char *message,
                                    int message_len) {
            std::unique_lock<std::mutex> l(lock_);

            // We don't log if the base_name_ is "" (which means "don't write")
            if (base_filename_selected_ && base_filename_.empty()) {
                return;
            }

            if (static_cast<int>(file_length_ >> 20) >= MaxLogSize() ||
                turbo::sysinfo::pid_has_changed()) {
                if (file_ != nullptr) fclose(file_);
                file_ = nullptr;
                file_length_ = bytes_since_flush_ = dropped_mem_length_ = 0;
                rollover_attempt_ = kRolloverAttemptFrequency - 1;
            }

            // If there's no destination file, make one before outputting
            if (file_ == nullptr) {
                // Try to rollover the log file every 32 log messages.  The only time
                // this could matter would be when we have trouble creating the log
                // file.  If that happens, we'll lose lots of log messages, of course!
                if (++rollover_attempt_ != kRolloverAttemptFrequency) return;
                rollover_attempt_ = 0;

                struct ::tm tm_time;
                if (FLAGS_turbo_log_utc_time)
                    gmtime_r(&timestamp, &tm_time);
                else
                    localtime_r(&timestamp, &tm_time);

                // The logfile's filename will have the date/time & pid in it
                ostringstream time_pid_stream;
                time_pid_stream.fill('0');
                time_pid_stream << 1900 + tm_time.tm_year
                                << setw(2) << 1 + tm_time.tm_mon
                                << setw(2) << tm_time.tm_mday
                                << '-'
                                << setw(2) << tm_time.tm_hour
                                << setw(2) << tm_time.tm_min
                                << setw(2) << tm_time.tm_sec
                                << '.'
                                << turbo::sysinfo::get_main_thread_pid();
                const string &time_pid_string = time_pid_stream.str();

                if (base_filename_selected_) {
                    if (!create_logfile(time_pid_string)) {
                        perror("Could not create log file");
                        fprintf(stderr, "COULD NOT CREATE LOGFILE '%s'!\n",
                                time_pid_string.c_str());
                        return;
                    }
                } else {
                    // If no base filename for logs of this severity has been set, use a
                    // default base filename of
                    // "<program name>.<hostname>.<user name>.log.<severity level>.".  So
                    // logfiles will have names like
                    // webserver.examplehost.root.log.INFO.19990817-150000.4354, where
                    // 19990817 is a date (1999 August 17), 150000 is a time (15:00:00),
                    // and 4354 is the pid of the logging process.  The date & time reflect
                    // when the file was created for output.
                    //
                    // Where does the file get put?  Successively try the directories
                    // "/tmp", and "."
                    string stripped_filename(
                            log_internal::program_invocation_short_name());
                    string hostname = turbo::sysinfo::get_host_name();

                    string uidname = turbo::sysinfo::user_name();
                    // We should not call TURBO_CHECK() here because this function can be
                    // called after holding on to log_mutex. We don't want to
                    // attempt to hold on to the same mutex, and get into a
                    // deadlock. Simply use a name like invalid-user.
                    if (uidname.empty()) uidname = "invalid-user";

                    stripped_filename = stripped_filename + '.' + hostname + '.'
                                        + uidname + ".log."
                                        + log_severity_names[severity_] + '.';
                    // We're going to (potentially) try to put logs in several different dirs
                    const vector<string> &log_dirs = GetLoggingDirectories();

                    // Go through the list of dirs, and try to create the log file in each
                    // until we succeed or run out of options
                    bool success = false;
                    for (vector<string>::const_iterator dir = log_dirs.begin();
                         dir != log_dirs.end();
                         ++dir) {
                        base_filename_ = *dir + "/" + stripped_filename;
                        if (create_logfile(time_pid_string)) {
                            success = true;
                            break;
                        }
                    }
                    // If we never succeeded, we have to give up
                    if (success == false) {
                        perror("Could not create logging file");
                        fprintf(stderr, "COULD NOT CREATE A LOGGINGFILE %s!",
                                time_pid_string.c_str());
                        return;
                    }
                }

                // Write a header message into the log file
                ostringstream file_header_stream;
                file_header_stream.fill('0');
                file_header_stream << "Log file created at: "
                                   << 1900 + tm_time.tm_year << '/'
                                   << setw(2) << 1 + tm_time.tm_mon << '/'
                                   << setw(2) << tm_time.tm_mday
                                   << ' '
                                   << setw(2) << tm_time.tm_hour << ':'
                                   << setw(2) << tm_time.tm_min << ':'
                                   << setw(2) << tm_time.tm_sec << (FLAGS_turbo_log_utc_time ? " UTC\n" : "\n")
                                   << "Running on machine: "
                                   << log_destination::hostname() << '\n';

                if (!g_application_fingerprint.empty()) {
                    file_header_stream << "Application fingerprint: " << g_application_fingerprint << '\n';
                }

                file_header_stream << "Running duration (h:mm:ss): "
                                   << pretty_duration(turbo::time_now() - start_time_)
                                   << '\n'
                                   << "Log line format: [IWEF]yyyymmdd hh:mm:ss.uuuuuu "
                                   << "threadid file:line] msg" << '\n';
                const string &file_header_string = file_header_stream.str();

                const int header_len = file_header_string.size();
                fwrite(file_header_string.data(), 1, header_len, file_);
                file_length_ += header_len;
                bytes_since_flush_ += header_len;
            }

            // Write to TURBO_LOG file
            if (!stop_writing) {
                // fwrite() doesn't return an error when the disk is full, for
                // messages that are less than 4096 bytes. When the disk is full,
                // it returns the message length for messages that are less than
                // 4096 bytes. fwrite() returns 4096 for message lengths that are
                // greater than 4096, thereby indicating an error.
                errno = 0;
                fwrite(message, 1, message_len, file_);
                if (FLAGS_turbo_stop_logging_if_full_disk &&
                    errno == ENOSPC) {  // disk full, stop writing to disk
                    stop_writing = true;  // until the disk is
                    return;
                } else {
                    file_length_ += message_len;
                    bytes_since_flush_ += message_len;
                }
            } else {
                if (turbo::time_now() >= next_flush_time_)
                    stop_writing = false;  // check to see if disk has free space.
                return;  // no need to flush
            }

            // See important msgs *now*.  Also, flush logs at least every 10^6 chars,
            // or every "FLAGS_turbo_logbufsecs" seconds.
            if (force_flush ||
                (bytes_since_flush_ >= 1000000) ||
                (turbo::time_now() >= next_flush_time_)) {
                flush_unlocked();
#ifdef TURBO_PLATFORM_LINUX
                // Only consider files >= 3MiB
        if (FLAGS_drop_log_memory && file_length_ >= (3 << 20)) {
          // Don't evict the most recent 1-2MiB so as not to impact a tailer
          // of the log file and to avoid page rounding issue on linux < 4.7
          uint32_t total_drop_length = (file_length_ & ~((1 << 20) - 1)) - (1 << 20);
          uint32_t this_drop_length = total_drop_length - dropped_mem_length_;
          if (this_drop_length >= (2 << 20)) {
            // Only advise when >= 2MiB to drop
# if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 21)
            // 'posix_fadvise' introduced in API 21:
            // * https://android.googlesource.com/platform/bionic/+/6880f936173081297be0dc12f687d341b86a4cfa/libc/libc.map.txt#732
# else
            posix_fadvise(fileno(file_), dropped_mem_length_, this_drop_length,
                          POSIX_FADV_DONTNEED);
# endif
            dropped_mem_length_ = total_drop_length;
          }
        }
#endif

                // Perform clean up for old logs
                if (g_log_cleaner.enabled()) {
                    if (base_filename_selected_ && base_filename_.empty()) {
                        return;
                    }
                    g_log_cleaner.run(base_filename_selected_,
                                      base_filename_,
                                      filename_extension_);
                }
            }
        }


        log_cleaner::log_cleaner() : enabled_(false), overdue_days_(7), dir_delim_('/') {

        }

        void log_cleaner::enable(int overdue_days) {
            // Setting overdue_days to 0 day should not be allowed!
            // Since all logs will be deleted immediately, which will cause troubles.
            assert(overdue_days > 0);

            enabled_ = true;
            overdue_days_ = overdue_days;
        }

        void log_cleaner::disable() {
            enabled_ = false;
        }

        void log_cleaner::run(bool base_filename_selected,
                              const string &base_filename,
                              const string &filename_extension) const {
            assert(enabled_ && overdue_days_ > 0);

            vector<string> dirs;

            if (base_filename_selected) {
                string dir = base_filename.substr(0, base_filename.find_last_of(dir_delim_) + 1);
                dirs.push_back(dir);
            } else {
                dirs = GetLoggingDirectories();
            }

            for (size_t i = 0; i < dirs.size(); i++) {
                vector<string> logs = get_overdue_log_names(dirs[i],
                                                            overdue_days_,
                                                            base_filename,
                                                            filename_extension);
                for (size_t j = 0; j < logs.size(); j++) {
                    static_cast<void>(unlink(logs[j].c_str()));
                }
            }
        }

        vector<string> log_cleaner::get_overdue_log_names(string log_directory,
                                                          int days,
                                                          const string &base_filename,
                                                          const string &filename_extension) const {
            // The names of overdue logs.
            vector<string> overdue_log_names;

            // Try to get all files within log_directory.
            DIR *dir;
            struct dirent *ent;

            // If log_directory doesn't end with a slash, append a slash to it.
            if (log_directory.at(log_directory.size() - 1) != dir_delim_) {
                log_directory += dir_delim_;
            }

            if ((dir = opendir(log_directory.c_str()))) {
                while ((ent = readdir(dir))) {
                    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
                        continue;
                    }
                    string filepath = log_directory + ent->d_name;
                    if (is_log_from_current_project(filepath, base_filename, filename_extension) &&
                        is_log_last_modified_over(filepath, days)) {
                        overdue_log_names.push_back(filepath);
                    }
                }
                closedir(dir);
            }

            return overdue_log_names;
        }

        bool log_cleaner::is_log_from_current_project(const string &filepath,
                                                      const string &base_filename,
                                                      const string &filename_extension) const {
            // We should remove duplicated delimiters from `base_filename`, e.g.,
            // before: "/tmp//<base_filename>.<create_time>.<pid>"
            // after:  "/tmp/<base_filename>.<create_time>.<pid>"
            string cleaned_base_filename;

            size_t real_filepath_size = filepath.size();
            for (size_t i = 0; i < base_filename.size(); ++i) {
                const char &c = base_filename[i];

                if (cleaned_base_filename.empty()) {
                    cleaned_base_filename += c;
                } else if (c != dir_delim_ ||
                           c != cleaned_base_filename.at(cleaned_base_filename.size() - 1)) {
                    cleaned_base_filename += c;
                }
            }

            // Return early if the filename doesn't start with `cleaned_base_filename`.
            if (filepath.find(cleaned_base_filename) != 0) {
                return false;
            }

            // Check if in the string `filename_extension` is right next to
            // `cleaned_base_filename` in `filepath` if the user
            // has set a custom filename extension.
            if (!filename_extension.empty()) {
                if (cleaned_base_filename.size() >= real_filepath_size) {
                    return false;
                }
                // for origin version, `filename_extension` is middle of the `filepath`.
                string ext = filepath.substr(cleaned_base_filename.size(), filename_extension.size());
                if (ext == filename_extension) {
                    cleaned_base_filename += filename_extension;
                } else {
                    // for new version, `filename_extension` is right of the `filepath`.
                    if (filename_extension.size() >= real_filepath_size) {
                        return false;
                    }
                    real_filepath_size = filepath.size() - filename_extension.size();
                    if (filepath.substr(real_filepath_size) != filename_extension) {
                        return false;
                    }
                }
            }

            // The characters after `cleaned_base_filename` should match the format:
            // YYYYMMDD-HHMMSS.pid
            for (size_t i = cleaned_base_filename.size(); i < real_filepath_size; i++) {
                const char &c = filepath[i];

                if (i <= cleaned_base_filename.size() + 7) { // 0 ~ 7 : YYYYMMDD
                    if (c < '0' || c > '9') { return false; }

                } else if (i == cleaned_base_filename.size() + 8) { // 8: -
                    if (c != '-') { return false; }

                } else if (i <= cleaned_base_filename.size() + 14) { // 9 ~ 14: HHMMSS
                    if (c < '0' || c > '9') { return false; }

                } else if (i == cleaned_base_filename.size() + 15) { // 15: .
                    if (c != '.') { return false; }

                } else if (i >= cleaned_base_filename.size() + 16) { // 16+: pid
                    if (c < '0' || c > '9') { return false; }
                }
            }

            return true;
        }

        bool log_cleaner::is_log_last_modified_over(const string &filepath, int days) const {
            // Try to get the last modified time of this file.
            struct stat file_stat;

            if (stat(filepath.c_str(), &file_stat) == 0) {
                // A day is 86400 seconds, so 7 days is 86400 * 7 = 604800 seconds.
                time_t last_modified_time = file_stat.st_mtime;
                time_t current_time = time(nullptr);
                return difftime(current_time, last_modified_time) > days * 86400;
            }

            // If failed to get file stat, don't return true!
            return false;
        }

    }  // namespace

// Static log data space to avoid alloc failures in a TURBO_LOG(FATAL)
//
// Since multiple threads may call TURBO_LOG(FATAL), and we want to preserve
// the data from the first call, we allocate two sets of space.  One
// for exclusive use by the first thread, and one for shared use by
// all other threads.
    static std::mutex fatal_msg_lock;
    static crash_reason crash_reason;
    static bool fatal_msg_exclusive = true;
    static log_message::log_message_data fatal_msg_data_exclusive;
    static log_message::log_message_data fatal_msg_data_shared;

    // Static thread-local log data space to use, because typically at most one
    // log_message_data object exists (in this case log makes zero heap memory
    // allocations).
    static __thread bool thread_data_available = true;

    static __thread
    std::aligned_storage<sizeof(log_message::log_message_data),
            alignof(log_message::log_message_data)>::type thread_msg_data;

    log_message::log_message_data::log_message_data()
            : stream_(message_text_, log_message::kMaxLogMessageLen, 0) {
    }

    log_message::log_message(const char *file, int line, log_severity severity,
                             uint64_t ctr, void (log_message::*send_method)())
            : allocated_(nullptr) {
        init(file, line, severity, send_method);
        data_->stream_.set_ctr(ctr);
    }

    log_message::log_message(const char *file, int line,
                             const CheckOpString &result)
            : allocated_(nullptr) {
        init(file, line, TURBO_FATAL, &log_message::send_to_log);
        stream() << "Check failed: " << (*result.str_) << " ";
    }

    log_message::log_message(const char *file, int line)
            : allocated_(nullptr) {
        init(file, line, TURBO_INFO, &log_message::send_to_log);
    }

    log_message::log_message(const char *file, int line, log_severity severity)
            : allocated_(nullptr) {
        init(file, line, severity, &log_message::send_to_log);
    }

    log_message::log_message(const char *file, int line, log_severity severity,
                             log_sink *sink, bool also_send_to_log)
            : allocated_(nullptr) {
        init(file, line, severity, also_send_to_log ? &log_message::send_to_sink_and_log :
                                   &log_message::send_to_sink);
        data_->sink_ = sink;  // override Init()'s setting to nullptr
    }

    log_message::log_message(const char *file, int line, log_severity severity,
                             vector<string> *outvec)
            : allocated_(nullptr) {
        init(file, line, severity, &log_message::save_or_send_to_log);
        data_->outvec_ = outvec; // override Init()'s setting to nullptr
    }

    log_message::log_message(const char *file, int line, log_severity severity,
                             string *message)
            : allocated_(nullptr) {
        init(file, line, severity, &log_message::write_to_string_and_log);
        data_->message_ = message;  // override Init()'s setting to nullptr
    }

    void log_message::init(const char *file,
                           int line,
                           log_severity severity,
                           void (log_message::*send_method)()) {
        allocated_ = nullptr;
        if (severity != TURBO_FATAL || !FLAGS_turbo_crash_on_fatal_log) {
            // No need for locking, because this is thread local.
            if (thread_data_available) {
                thread_data_available = false;
                data_ = new(&thread_msg_data) log_message_data;
            } else {
                allocated_ = new log_message_data();
                data_ = allocated_;
            }
            data_->first_fatal_ = false;
        } else {
            std::unique_lock<std::mutex> l(fatal_msg_lock);
            if (fatal_msg_exclusive) {
                fatal_msg_exclusive = false;
                data_ = &fatal_msg_data_exclusive;
                data_->first_fatal_ = true;
            } else {
                data_ = &fatal_msg_data_shared;
                data_->first_fatal_ = false;
            }
        }

        stream().fill('0');
        data_->preserved_errno_ = errno;
        data_->severity_ = severity;
        data_->line_ = line;
        data_->send_method_ = send_method;
        data_->sink_ = nullptr;
        data_->outvec_ = nullptr;
        auto tv = turbo::time_now().to_timeval();
        data_->timestamp_ = static_cast<time_t>(tv.tv_sec);
        if (FLAGS_turbo_log_utc_time)
            gmtime_r(&data_->timestamp_, &data_->tm_time_);
        else
            localtime_r(&data_->timestamp_, &data_->tm_time_);
        data_->usecs_ = static_cast<int32_t>((tv.tv_usec));

        data_->num_chars_to_log_ = 0;
        data_->num_chars_to_syslog_ = 0;
        data_->basename_ = const_basename(file);
        data_->fullname_ = file;
        data_->has_been_flushed_ = false;

        // If specified, prepend a prefix to each line.  For example:
        //    I20201018 160715 f5d4fbb0 logging.cc:1153]
        //    (log level, GMT year, month, date, time, thread_id, file basename, line)
        // We exclude the thread_id for the default thread.
        if (FLAGS_turbo_log_prefix && (line != kNoLogPrefix)) {
            stream() << log_severity_names[severity][0]
                     << setw(4) << 1900 + data_->tm_time_.tm_year
                     << setw(2) << 1 + data_->tm_time_.tm_mon
                     << setw(2) << data_->tm_time_.tm_mday
                     << ' '
                     << setw(2) << data_->tm_time_.tm_hour << ':'
                     << setw(2) << data_->tm_time_.tm_min << ':'
                     << setw(2) << data_->tm_time_.tm_sec << "."
                     << setw(6) << data_->usecs_
                     << ' '
                     << setfill(' ') << setw(5)
                     << static_cast<unsigned int>(turbo::thread::thread_index()) << setfill('0')
                     << ' '
                     << data_->basename_ << ':' << data_->line_ << "] ";
        }
        data_->num_prefix_chars_ = data_->stream_.pcount();

        if (!FLAGS_turbo_log_backtrace_at.empty()) {
            char fileline[128];
            snprintf(fileline, sizeof(fileline), "%s:%d", data_->basename_, line);
#ifdef HAVE_STACKTRACE
            if (!strcmp(FLAGS_turbo_log_backtrace_at.c_str(), fileline)) {
                string stacktrace;
                dump_stack_trace_to_string(&stacktrace);
                stream() << " (stacktrace:\n" << stacktrace << ") ";
            }
#endif
        }
    }

    log_message::~log_message() {
        flush();
        if (data_ == static_cast<void *>(&thread_msg_data)) {
            data_->~log_message_data();
            thread_data_available = true;
        } else {
            delete allocated_;
        }
    }

    int log_message::preserved_errno() const {
        return data_->preserved_errno_;
    }

    ostream &log_message::stream() {
        return data_->stream_;
    }

    namespace {
#if defined(__ANDROID__)
        int AndroidLogLevel(const int severity) {
      switch (severity) {
        case 3:
          return ANDROID_LOG_FATAL;
        case 2:
          return ANDROID_LOG_ERROR;
        case 1:
          return ANDROID_LOG_WARN;
        default:
          return ANDROID_LOG_INFO;
      }
    }
#endif  // defined(__ANDROID__)
    }  // namespace

    // Flush buffered message, called by the destructor, or any other function
    // that needs to synchronize the log.
    void log_message::flush() {
        if (data_->has_been_flushed_ || data_->severity_ < FLAGS_turbo_minloglevel)
            return;

        data_->num_chars_to_log_ = data_->stream_.pcount();
        data_->num_chars_to_syslog_ =
                data_->num_chars_to_log_ - data_->num_prefix_chars_;

        // Do we need to add a \n to the end of this message?
        bool append_newline =
                (data_->message_text_[data_->num_chars_to_log_ - 1] != '\n');
        char original_final_char = '\0';

        // If we do need to add a \n, we'll do it by violating the memory of the
        // ostrstream buffer.  This is quick, and we'll make sure to undo our
        // modification before anything else is done with the ostrstream.  It
        // would be preferable not to do things this way, but it seems to be
        // the best way to deal with this.
        if (append_newline) {
            original_final_char = data_->message_text_[data_->num_chars_to_log_];
            data_->message_text_[data_->num_chars_to_log_++] = '\n';
        }
        data_->message_text_[data_->num_chars_to_log_] = '\0';

        // Prevent any subtle race conditions by wrapping a mutex lock around
        // the actual logging action per se.
        {
            std::unique_lock<std::mutex> l(log_mutex);
            (this->*(data_->send_method_))();
            ++num_messages_[static_cast<int>(data_->severity_)];
        }
        log_destination::wait_for_sinks(data_);

#if defined(__ANDROID__)
        const int level = AndroidLogLevel((int)data_->severity_);
        const std::string text = std::string(data_->message_text_);
        __android_log_write(level, "native", text.substr(0,data_->num_chars_to_log_).c_str());
#endif  // defined(__ANDROID__)

        if (append_newline) {
            // Fix the ostrstream back how it was before we screwed with it.
            // It's 99.44% certain that we don't need to worry about doing this.
            data_->message_text_[data_->num_chars_to_log_ - 1] = original_final_char;
        }

        // If errno was already set before we enter the logging call, we'll
        // set it back to that value when we return from the logging call.
        // It happens often that we log an error message after a syscall
        // failure, which can potentially set the errno to some other
        // values.  We would like to preserve the original errno.
        if (data_->preserved_errno_ != 0) {
            errno = data_->preserved_errno_;
        }

        // Note that this message is now safely logged.  If we're asked to flush
        // again, as a result of destruction, say, we'll do nothing on future calls.
        data_->has_been_flushed_ = true;
    }

    // Copy of first FATAL log message so that we can print it out again
    // after all the stack traces.  To preserve legacy behavior, we don't
    // use fatal_msg_data_exclusive.
    static time_t fatal_time;
    static char fatal_message[256];

    void reprint_fatal_message() {
        if (fatal_message[0]) {
            const int n = strlen(fatal_message);
            if (!FLAGS_turbo_logtostderr) {
                // Also write to stderr (don't color to avoid terminal checks)
                write_to_stderr(fatal_message, n);
            }
            log_destination::log_to_all_logfiles(TURBO_ERROR, fatal_time, fatal_message, n);
        }
    }

// L >= log_mutex (callers must hold the log_mutex).
    void log_message::send_to_log() EXCLUSIVE_LOCKS_REQUIRED(log_mutex) {
        static bool already_warned_before_initgoogle = false;

        TURBO_RAW_DCHECK(data_->num_chars_to_log_ > 0 &&
                   data_->message_text_[data_->num_chars_to_log_ - 1] == '\n', "");

        // Messages of a given severity get logged to lower severity logs, too

        if (!already_warned_before_initgoogle && !is_logging_initialized()) {
            const char w[] = "WARNING: Logging before init_logging() is "
                             "written to STDERR\n";
            write_to_stderr(w, strlen(w));
            already_warned_before_initgoogle = true;
        }

        // global flag: never log to file if set.  Also -- don't log to a
        // file if we haven't parsed the command line flags to get the
        // program name.
        if (FLAGS_turbo_logtostderr || !is_logging_initialized()) {
            colored_write_to_stderr(data_->severity_,
                                    data_->message_text_, data_->num_chars_to_log_);

            // this could be protected by a flag if necessary.
            log_destination::log_to_sinks(data_->severity_,
                                          data_->fullname_, data_->basename_,
                                          data_->line_, &data_->tm_time_,
                                          data_->message_text_ + data_->num_prefix_chars_,
                                          (data_->num_chars_to_log_ -
                                           data_->num_prefix_chars_ - 1),
                                          data_->usecs_);
        } else {

            // log this message to all log files of severity <= severity_
            log_destination::log_to_all_logfiles(data_->severity_, data_->timestamp_,
                                                 data_->message_text_,
                                                 data_->num_chars_to_log_);

            log_destination::maybe_log_to_stderr(data_->severity_, data_->message_text_,
                                                 data_->num_chars_to_log_,
                                                 data_->num_prefix_chars_);
            log_destination::maybe_log_to_email(data_->severity_, data_->message_text_,
                                                data_->num_chars_to_log_);
            log_destination::log_to_sinks(data_->severity_,
                                          data_->fullname_, data_->basename_,
                                          data_->line_, &data_->tm_time_,
                                          data_->message_text_ + data_->num_prefix_chars_,
                                          (data_->num_chars_to_log_
                                           - data_->num_prefix_chars_ - 1),
                                          data_->usecs_);
            // NOTE: -1 removes trailing \n
        }

        // If we log a FATAL message, flush all the log destinations, then toss
        // a signal for others to catch. We leave the logs in a state that
        // someone else can use them (as long as they flush afterwards)
        if (data_->severity_ == TURBO_FATAL && FLAGS_turbo_crash_on_fatal_log) {
            if (data_->first_fatal_) {
                // Store crash information so that it is accessible from within signal
                // handlers that may be invoked later.
                record_crash_reason(&crash_reason);
                set_crash_reason(&crash_reason);

                // Store shortened fatal message for other logs and GWQ status
                const int copy = min<int>(data_->num_chars_to_log_,
                                          sizeof(fatal_message) - 1);
                memcpy(fatal_message, data_->message_text_, copy);
                fatal_message[copy] = '\0';
                fatal_time = data_->timestamp_;
            }

            if (!FLAGS_turbo_logtostderr) {
                for (int i = 0; i < NUM_SEVERITIES; ++i) {
                    if (log_destination::log_destinations_[i])
                        log_destination::log_destinations_[i]->logger_->write(true, 0, "", 0);
                }
            }

            // release the lock that our caller (directly or indirectly)
            // log_message::~log_message() grabbed so that signal handlers
            // can use the logging facility. Alternately, we could add
            // an entire unsafe logging interface to bypass locking
            // for signal handlers but this seems simpler.
            log_mutex.unlock();
            log_destination::wait_for_sinks(data_);

            const char *message = "*** Check failure stack trace: ***\n";
            if (write(STDERR_FILENO, message, strlen(message)) < 0) {
                // Ignore errors.
            }
            fail();
        }
    }

    void log_message::record_crash_reason(
            log_internal::crash_reason *reason) {
        reason->filename = fatal_msg_data_exclusive.fullname_;
        reason->line_number = fatal_msg_data_exclusive.line_;
        reason->message = fatal_msg_data_exclusive.message_text_ +
                          fatal_msg_data_exclusive.num_prefix_chars_;
#ifdef HAVE_STACKTRACE
        // Retrieve the stack trace, omitting the logging frames that got us here.
        reason->depth = turbo::debugging::get_stack_trace(reason->stack, TURBO_ARRAY_SIZE(reason->stack), 4);
#else
        reason->depth = 0;
#endif
    }

#ifdef HAVE___ATTRIBUTE__
# define ATTRIBUTE_NORETURN __attribute__((noreturn))
#else
# define ATTRIBUTE_NORETURN
#endif


    static void logging_fail() ATTRIBUTE_NORETURN;

    static void logging_fail() {
        if (FLAGS_turbo_crash_on_fatal_log) {
            abort();
        }
    }

    typedef void (*logging_fail_func_t)() ATTRIBUTE_NORETURN;

    TURBO_EXPORT logging_fail_func_t g_logging_fail_func = &logging_fail;

    void InstallFailureFunction(void (*fail_func)()) {
        g_logging_fail_func = (logging_fail_func_t) fail_func;
    }

    void log_message::fail() {
        g_logging_fail_func();
    }

// L >= log_mutex (callers must hold the log_mutex).
    void log_message::send_to_sink() EXCLUSIVE_LOCKS_REQUIRED(log_mutex) {
        if (data_->sink_ != nullptr) {
            TURBO_RAW_DCHECK(data_->num_chars_to_log_ > 0 &&
                       data_->message_text_[data_->num_chars_to_log_ - 1] == '\n', "");
            data_->sink_->send(data_->severity_, data_->fullname_, data_->basename_,
                               data_->line_, &data_->tm_time_,
                               data_->message_text_ + data_->num_prefix_chars_,
                               (data_->num_chars_to_log_ -
                                data_->num_prefix_chars_ - 1),
                               data_->usecs_);
        }
    }

// L >= log_mutex (callers must hold the log_mutex).
    void log_message::send_to_sink_and_log() EXCLUSIVE_LOCKS_REQUIRED(log_mutex) {
        send_to_sink();
        send_to_log();
    }

// L >= log_mutex (callers must hold the log_mutex).
    void log_message::save_or_send_to_log() EXCLUSIVE_LOCKS_REQUIRED(log_mutex) {
        if (data_->outvec_ != nullptr) {
            TURBO_RAW_DCHECK(data_->num_chars_to_log_ > 0 &&
                       data_->message_text_[data_->num_chars_to_log_ - 1] == '\n', "");
            // Omit prefix of message and trailing newline when recording in outvec_.
            const char *start = data_->message_text_ + data_->num_prefix_chars_;
            int len = data_->num_chars_to_log_ - data_->num_prefix_chars_ - 1;
            data_->outvec_->push_back(string(start, len));
        } else {
            send_to_log();
        }
    }

    void log_message::write_to_string_and_log() EXCLUSIVE_LOCKS_REQUIRED(log_mutex) {
        if (data_->message_ != nullptr) {
            TURBO_RAW_DCHECK(data_->num_chars_to_log_ > 0 &&
                       data_->message_text_[data_->num_chars_to_log_ - 1] == '\n', "");
            // Omit prefix of message and trailing newline when writing to message_.
            const char *start = data_->message_text_ + data_->num_prefix_chars_;
            int len = data_->num_chars_to_log_ - data_->num_prefix_chars_ - 1;
            data_->message_->assign(start, len);
        }
        send_to_log();
    }

    // L >= log_mutex (callers must hold the log_mutex).
    void log_message::send_to_syslog_and_log() {
        // Before any calls to syslog(), make a single call to openlog()
        static bool openlog_already_called = false;
        if (!openlog_already_called) {
            openlog(log_internal::program_invocation_short_name(),
                    LOG_CONS | LOG_NDELAY | LOG_PID,
                    LOG_USER);
            openlog_already_called = true;
        }

        // This array maps Google severity levels to syslog levels
        const int SEVERITY_TO_LEVEL[] = {LOG_INFO, LOG_WARNING, LOG_ERR, LOG_EMERG};
        syslog(LOG_USER | SEVERITY_TO_LEVEL[static_cast<int>(data_->severity_)], "%.*s",
               int(data_->num_chars_to_syslog_),
               data_->message_text_ + data_->num_prefix_chars_);
        send_to_log();
    }

    base::inner_logger *base::get_logger(log_severity severity) {
        std::unique_lock<std::mutex> l(log_mutex);
        return log_destination::get_log_destination(severity)->logger_;
    }

    void base::set_logger(log_severity severity, base::inner_logger *logger) {
        std::unique_lock<std::mutex> l(log_mutex);
        log_destination::get_log_destination(severity)->logger_ = logger;
    }

// L < log_mutex.  Acquires and releases mutex_.
    int64_t log_message::num_messages(int severity) {
        std::unique_lock<std::mutex> l(log_mutex);
        return num_messages_[severity];
    }

// Output the COUNTER value. This is only valid if ostream is a
// log_stream.
    ostream &operator<<(ostream &os, const PRIVATE_Counter &) {
#ifdef DISABLE_RTTI
        log_message::log_stream *log = static_cast<log_message::log_stream*>(&os);
#else
        log_message::log_stream *log = dynamic_cast<log_message::log_stream *>(&os);
#endif
        TURBO_CHECK(log && log == log->self())
                        << "You must not use COUNTER with non-log ostream";
        os << log->ctr();
        return os;
    }

    errno_log_message::errno_log_message(const char *file, int line,
                                         log_severity severity, uint64_t ctr,
                                         void (log_message::*send_method)())
            : log_message(file, line, severity, ctr, send_method) {
    }

    errno_log_message::~errno_log_message() {
        // Don't access errno directly because it may have been altered
        // while streaming the message.
        stream() << ": " << StrError(preserved_errno()) << " ["
                 << preserved_errno() << "]";
    }

    void flush_log_files(log_severity min_severity) {
        log_destination::flush_log_files(min_severity);
    }

    void flush_log_files_unsafe(log_severity min_severity) {
        log_destination::flush_log_files_unsafe(min_severity);
    }

    void set_log_destination(log_severity severity, const char *base_filename) {
        log_destination::set_log_destination(severity, base_filename);
    }

    void set_log_symlink(log_severity severity, const char *symlink_basename) {
        log_destination::set_log_symlink(severity, symlink_basename);
    }

    log_sink::~log_sink() {
    }

    void log_sink::WaitTillSent() {
        // noop default
    }

    string log_sink::ToString(log_severity severity, const char *file, int line,
                              const struct ::tm *tm_time,
                              const char *message, size_t message_len, int32_t usecs) {
        ostringstream stream(string(message, message_len));
        stream.fill('0');

        stream << log_severity_names[severity][0]
               << setw(4) << 1900 + tm_time->tm_year
               << setw(2) << 1 + tm_time->tm_mon
               << setw(2) << tm_time->tm_mday
               << ' '
               << setw(2) << tm_time->tm_hour << ':'
               << setw(2) << tm_time->tm_min << ':'
               << setw(2) << tm_time->tm_sec << '.'
               << setw(6) << usecs
               << ' '
               << setfill(' ') << setw(5) << turbo::sysinfo::get_tid() << setfill('0')
               << ' '
               << file << ':' << line << "] ";

        stream << string(message, message_len);
        return stream.str();
    }

    void add_log_sink(log_sink *destination) {
        log_destination::add_log_sink(destination);
    }

    void remove_log_sink(log_sink *destination) {
        log_destination::remove_log_sink(destination);
    }

    void set_log_filename_extension(const char *ext) {
        log_destination::set_log_filename_extension(ext);
    }

    void set_stderr_logging(log_severity min_severity) {
        log_destination::set_stderr_logging(min_severity);
    }

    void set_email_logging(log_severity min_severity, const char *addresses) {
        log_destination::set_email_logging(min_severity, addresses);
    }

    void log_to_stderr() {
        log_destination::log_to_stderr();
    }

    // Shell-escaping as we need to shell out ot /bin/mail.
    static const char kDontNeedShellEscapeChars[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+-_.=/:,@";

    static string ShellEscape(const string &src) {
        string result;
        if (!src.empty() &&  // empty string needs quotes
            src.find_first_not_of(kDontNeedShellEscapeChars) == string::npos) {
            // only contains chars that don't need quotes; it's fine
            result.assign(src);
        } else if (src.find_first_of('\'') == string::npos) {
            // no single quotes; just wrap it in single quotes
            result.assign("'");
            result.append(src);
            result.append("'");
        } else {
            // needs double quote escaping
            result.assign("\"");
            for (size_t i = 0; i < src.size(); ++i) {
                switch (src[i]) {
                    case '\\':
                    case '$':
                    case '"':
                    case '`':
                        result.append("\\");
                }
                result.append(src, i, 1);
            }
            result.append("\"");
        }
        return result;
    }


    // use_logging controls whether the logging functions TURBO_LOG/TURBO_VLOG are used
    // to log errors.  It should be set to false when the caller holds the
    // log_mutex.
    static bool SendEmailInternal(const char *dest, const char *subject,
                                  const char *body, bool use_logging) {
        if (dest && *dest) {
            if (use_logging) {
                TURBO_VLOG(1) << "Trying to send TITLE:" << subject
                        << " BODY:" << body << " to " << dest;
            } else {
                fprintf(stderr, "Trying to send TITLE: %s BODY: %s to %s\n",
                        subject, body, dest);
            }

            std::string turbo_log_mailer = FLAGS_turbo_log_mailer;
            if (turbo_log_mailer.empty()) {
                turbo_log_mailer = "/bin/mail";
            }
            std::string cmd =
                    turbo_log_mailer + " -s" +
                    ShellEscape(subject) + " " + ShellEscape(dest);
            TURBO_VLOG(4) << "Mailing command: " << cmd;

            FILE *pipe = popen(cmd.c_str(), "w");
            if (pipe != nullptr) {
                // Add the body if we have one
                if (body)
                    fwrite(body, sizeof(char), strlen(body), pipe);
                bool ok = pclose(pipe) != -1;
                if (!ok) {
                    if (use_logging) {
                        TURBO_LOG(ERROR) << "Problems sending mail to " << dest << ": "
                                   << StrError(errno);
                    } else {
                        fprintf(stderr, "Problems sending mail to %s: %s\n",
                                dest, StrError(errno).c_str());
                    }
                }
                return ok;
            } else {
                if (use_logging) {
                    TURBO_LOG(ERROR) << "Unable to send mail to " << dest;
                } else {
                    fprintf(stderr, "Unable to send mail to %s\n", dest);
                }
            }
        }
        return false;
    }

    bool SendEmail(const char *dest, const char *subject, const char *body) {
        return SendEmailInternal(dest, subject, body, true);
    }

    static void GetTempDirectories(vector<string> *list) {
        list->clear();
        // Directories, in order of preference. If we find a dir that
        // exists, we stop adding other less-preferred dirs
        const char *candidates[] = {
                // Non-null only during unittest/regtest
                getenv("TEST_TMPDIR"),

                // Explicitly-supplied temp dirs
                getenv("TMPDIR"), getenv("TMP"),

                // If all else fails
                "/tmp",
        };

        for (size_t i = 0; i < TURBO_ARRAY_SIZE(candidates); i++) {
            const char *d = candidates[i];
            if (!d) continue;  // Empty env var

            // Make sure we don't surprise anyone who's expecting a '/'
            string dstr = d;
            if (dstr[dstr.size() - 1] != '/') {
                dstr += "/";
            }
            list->push_back(dstr);

            struct stat statbuf;
            if (!stat(d, &statbuf) && S_ISDIR(statbuf.st_mode)) {
                // We found a dir that exists - we're done.
                return;
            }
        }
    }

    static vector<string> *logging_directories_list;

    const vector<string> &GetLoggingDirectories() {
        // Not strictly thread-safe but we're called early in InitGoogle().
        if (logging_directories_list == nullptr) {
            logging_directories_list = new vector<string>;

            if (!FLAGS_turbo_log_dir.empty()) {
                // A dir was specified, we should use it
                logging_directories_list->push_back(FLAGS_turbo_log_dir.c_str());
            } else {
                GetTempDirectories(logging_directories_list);
                logging_directories_list->push_back("./");
            }
        }
        return *logging_directories_list;
    }

    void TestOnly_ClearLoggingDirectoriesList() {
        fprintf(stderr, "TestOnly_ClearLoggingDirectoriesList should only be "
                        "called from test code.\n");
        delete logging_directories_list;
        logging_directories_list = nullptr;
    }

    void GetExistingTempDirectories(vector<string> *list) {
        GetTempDirectories(list);
        vector<string>::iterator i_dir = list->begin();
        while (i_dir != list->end()) {
            // zero arg to access means test for existence; no constant
            // defined on windows
            if (access(i_dir->c_str(), 0)) {
                i_dir = list->erase(i_dir);
            } else {
                ++i_dir;
            }
        }
    }

    void truncate_log_file(const char *path, int64_t limit, int64_t keep) {

        struct stat statbuf;
        const int kCopyBlockSize = 8 << 10;
        char copybuf[kCopyBlockSize];
        int64_t read_offset, write_offset;
        // Don't follow symlinks unless they're our own fd symlinks in /proc
        int flags = O_RDWR;
        // TODO(hamaji): Support other environments.
#ifdef TURBO_PLATFORM_LINUX
        const char *procfd_prefix = "/proc/self/fd/";
        if (strncmp(procfd_prefix, path, strlen(procfd_prefix))) flags |= O_NOFOLLOW;
#endif

        int fd = open(path, flags);
        if (fd == -1) {
            if (errno == EFBIG) {
                // The log file in question has got too big for us to open. The
                // real fix for this would be to compile logging.cc (or probably
                // all of base/...) with -D_FILE_OFFSET_BITS=64 but that's
                // rather scary.
                // Instead just truncate the file to something we can manage
                if (truncate(path, 0) == -1) {
                    TURBO_PLOG(ERROR) << "Unable to truncate " << path;
                } else {
                    TURBO_LOG(ERROR) << "Truncated " << path << " due to EFBIG error";
                }
            } else {
                TURBO_PLOG(ERROR) << "Unable to open " << path;
            }
            return;
        }

        if (fstat(fd, &statbuf) == -1) {
            TURBO_PLOG(ERROR) << "Unable to fstat()";
            goto out_close_fd;
        }

        // See if the path refers to a regular file bigger than the
        // specified limit
        if (!S_ISREG(statbuf.st_mode)) goto out_close_fd;
        if (statbuf.st_size <= limit) goto out_close_fd;
        if (statbuf.st_size <= keep) goto out_close_fd;

        // This log file is too large - we need to truncate it
        TURBO_LOG(INFO) << "Truncating " << path << " to " << keep << " bytes";

        // Copy the last "keep" bytes of the file to the beginning of the file
        read_offset = statbuf.st_size - keep;
        write_offset = 0;
        int bytesin, bytesout;
        while ((bytesin = ::pread(fd, copybuf, sizeof(copybuf), read_offset)) > 0) {
            bytesout = pwrite(fd, copybuf, bytesin, write_offset);
            if (bytesout == -1) {
                TURBO_PLOG(ERROR) << "Unable to write to " << path;
                break;
            } else if (bytesout != bytesin) {
                TURBO_LOG(ERROR) << "Expected to write " << bytesin << ", wrote " << bytesout;
            }
            read_offset += bytesin;
            write_offset += bytesout;
        }
        if (bytesin == -1) TURBO_PLOG(ERROR) << "Unable to read from " << path;

        // Truncate the remainder of the file. If someone else writes to the
        // end of the file after our last read() above, we lose their latest
        // data. Too bad ...
        if (ftruncate(fd, write_offset) == -1) {
            TURBO_PLOG(ERROR) << "Unable to truncate " << path;
        }

        out_close_fd:
        close(fd);
    }

    void truncate_stdout_stderr() {
        int64_t limit = MaxLogSize() << 20;
        int64_t keep = 1 << 20;
        truncate_log_file("/proc/self/fd/1", limit, keep);
        truncate_log_file("/proc/self/fd/2", limit, keep);
    }


// Helper functions for string comparisons.
#define DEFINE_CHECK_STROP_IMPL(name, func, expected)                   \
  string* Check##func##expected##Impl(const char* s1, const char* s2,   \
                                      const char* names) {              \
    bool equal = s1 == s2 || (s1 && s2 && !func(s1, s2));               \
    if (equal == expected) return nullptr;                                 \
    else {                                                              \
      ostringstream ss;                                                 \
      if (!s1) s1 = "";                                                 \
      if (!s2) s2 = "";                                                 \
      ss << #name " failed: " << names << " (" << s1 << " vs. " << s2 << ")"; \
      return new string(ss.str());                                      \
    }                                                                   \
  }

    DEFINE_CHECK_STROP_IMPL(CHECK_STREQ, strcmp, true)

    DEFINE_CHECK_STROP_IMPL(CHECK_STRNE, strcmp, false)

    DEFINE_CHECK_STROP_IMPL(CHECK_STRCASEEQ, strcasecmp, true)

    DEFINE_CHECK_STROP_IMPL(CHECK_STRCASENE, strcasecmp, false)

#undef DEFINE_CHECK_STROP_IMPL

    int posix_strerror_r(int err, char *buf, size_t len) {
        // Sanity check input parameters
        if (buf == nullptr || len <= 0) {
            errno = EINVAL;
            return -1;
        }

        // Reset buf and errno, and try calling whatever version of strerror_r()
        // is implemented by glibc
        buf[0] = '\000';
        int old_errno = errno;
        errno = 0;
        char *rc = reinterpret_cast<char *>(strerror_r(err, buf, len));

        // Both versions set errno on failure
        if (errno) {
            // Should already be there, but better safe than sorry
            buf[0] = '\000';
            return -1;
        }
        errno = old_errno;

        // POSIX is vague about whether the string will be terminated, although
        // is indirectly implies that typically ERANGE will be returned, instead
        // of truncating the string. This is different from the GNU implementation.
        // We play it safe by always terminating the string explicitly.
        buf[len - 1] = '\000';

        // If the function succeeded, we can use its exit code to determine the
        // semantics implemented by glibc
        if (!rc) {
            return 0;
        } else {
            // GNU semantics detected
            if (rc == buf) {
                return 0;
            } else {
                buf[0] = '\000';
#if defined(TURBO_PLATFORM_OSX) || defined(TURBO_PLATFORM_FREEBSD) || defined(TURBO_PLATFORM_OPENBSD)
                if (reinterpret_cast<intptr_t>(rc) < sys_nerr) {
                    // This means an error on MacOSX or FreeBSD.
                    return -1;
                }
#endif
                strncat(buf, rc, len - 1);
                return 0;
            }
        }
    }

    string StrError(int err) {
        char buf[100];
        int rc = posix_strerror_r(err, buf, sizeof(buf));
        if ((rc < 0) || (buf[0] == '\000')) {
            snprintf(buf, sizeof(buf), "Error number %d", err);
        }
        return buf;
    }

    log_message_fatal::log_message_fatal(const char *file, int line) :
            log_message(file, line, TURBO_FATAL) {}

    log_message_fatal::log_message_fatal(const char *file, int line,
                                         const CheckOpString &result) :
            log_message(file, line, result) {}

    log_message_fatal::~log_message_fatal() {
        flush();
        log_message::fail();
    }

    namespace base {

        CheckOpMessageBuilder::CheckOpMessageBuilder(const char *exprtext)
                : stream_(new ostringstream) {
            *stream_ << exprtext << " (";
        }

        CheckOpMessageBuilder::~CheckOpMessageBuilder() {
            delete stream_;
        }

        ostream *CheckOpMessageBuilder::ForVar2() {
            *stream_ << " vs. ";
            return stream_;
        }

        string *CheckOpMessageBuilder::NewString() {
            *stream_ << ")";
            return new string(stream_->str());
        }

    }  // namespace base

    template<>
    void make_check_op_value_string(std::ostream *os, const char &v) {
        if (v >= 32 && v <= 126) {
            (*os) << "'" << v << "'";
        } else {
            (*os) << "char value " << (short) v;
        }
    }

    template<>
    void make_check_op_value_string(std::ostream *os, const signed char &v) {
        if (v >= 32 && v <= 126) {
            (*os) << "'" << v << "'";
        } else {
            (*os) << "signed char value " << (short) v;
        }
    }

    template<>
    void make_check_op_value_string(std::ostream *os, const unsigned char &v) {
        if (v >= 32 && v <= 126) {
            (*os) << "'" << v << "'";
        } else {
            (*os) << "unsigned char value " << (unsigned short) v;
        }
    }

    template<>
    void make_check_op_value_string(std::ostream *os, const std::nullptr_t &v) {
        (*os) << "nullptr";
    }

    void init_logging(const char *argv0) {
        log_internal::init_logging_utilities(argv0);
    }

    void shutdown_logging() {
        log_internal::shutdown_logging_utilities();
        log_destination::delete_log_destinations();
        delete logging_directories_list;
        logging_directories_list = nullptr;
    }

    void enable_log_cleaner(int overdue_days) {
        g_log_cleaner.enable(overdue_days);
    }

    void disable_log_cleaner() {
        g_log_cleaner.disable();
    }

}  // namespace turbo::log


#include <string>
#include <vector>

namespace turbo::internal::logging {
/*
    namespace {

        std::vector<PrefixAppender *> *GetProviders() {
            // Not using `NeverDestroyed` as we're "low-level" utility and therefore
            // should not bring in too many dependencies..
            static std::vector<PrefixAppender *> providers;
            return &providers;
        }

    }  // namespace

    void InstallPrefixProvider(PrefixAppender *cb) {
        GetProviders()->push_back(cb);
    }

    void WritePrefixTo(std::string *to) {
        for (auto &&e : *GetProviders()) {
            auto was = to->size();
            e(to);
            if (to->size() != was) {
                to->push_back(' ');
            }
        }
    }
*/
}  // namespace turbo::internal::logging

