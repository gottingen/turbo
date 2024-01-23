// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <pthread.h>
#include <set>                                  // std::set
#include <fstream>                              // std::ifstream
#include <sstream>                              // std::ostringstream
#include <mutex>
#include "turbo/container/flat_hash_map.h"
#include "turbo/strings/utility.h"
#include "turbo/times/time.h"                          // milliseconds_from_now
#include "turbo/flags/flag.h"
#include "turbo/system/threading.h"
#include "turbo/var/variable.h"
#include "turbo/concurrent/call_once.h"
#include "turbo/base/internal/raw_logging.h"

TURBO_FLAG(bool, var_save_series, true,
           "Save values of last 60 seconds, last 60 minutes,"
           " last 24 hours and last 30 days for ploting");

TURBO_FLAG(bool, var_quote_vector, true,
           "Quote description of Vector<> to make it valid to noah");

TURBO_FLAG(bool, var_abort_on_same_name, false,
           "Abort when names of var are same");

TURBO_FLAG(bool, var_log_dumpped, false,
           "[For debugging] print dumpped info"
           " into logstream before call Dumpper");

namespace turbo {

    static bool s_var_may_abort = false;

    const size_t SUB_MAP_COUNT = 32;
    static_assert(!(SUB_MAP_COUNT & (SUB_MAP_COUNT - 1)), " must be power of 2");

    class VarEntry {
    public:
        VarEntry() : var(nullptr), display_filter(DISPLAY_ON_ALL) {}

        Variable *var;
        DisplayFilter display_filter;
    };

    typedef turbo::flat_hash_map<std::string, VarEntry> VarMap;

    struct VarMapWithLock : public VarMap {
        std::mutex mutex;

        VarMapWithLock() : VarMap() {}
    };

    static once_flag s_var_maps_once;
    static VarMapWithLock *s_var_maps = nullptr;

    static void init_var_maps() {
        // It's probably slow to initialize all sub maps, but rpc often expose
        // variables before user. So this should not be an issue to users.
        s_var_maps = new VarMapWithLock[SUB_MAP_COUNT];
    }

    inline size_t sub_map_index(const std::string_view &str) {
        if (str.empty()) {
            return 0;
        }
        size_t h = 0;
        for (auto p: str) {
            h = h * 5 + p;
        }
        return h & (SUB_MAP_COUNT - 1);
    }

    inline VarMapWithLock *get_var_maps() {
        turbo::call_once(s_var_maps_once, init_var_maps);
        return s_var_maps;
    }

    inline VarMapWithLock &get_var_map(const std::string_view &name) {
        VarMapWithLock &m = get_var_maps()[sub_map_index(name)];
        return m;
    }

    Variable::~Variable() {
        TURBO_RAW_CHECK(!hide(), "Subclass of Variable MUST call hide() manually in their"
                                 " dtors to avoid displaying a variable that is just destructing");
    }

    int Variable::expose_impl(const std::string_view &prefix,
                              const std::string_view &name,
                              DisplayFilter display_filter) {
        if (name.empty()) {
            TURBO_RAW_LOG(ERROR, "Parameter[name] is empty");
            return -1;
        }
        // NOTE: It's impossible to atomically erase from a submap and insert into
        // another submap without a global lock. When the to-be-exposed name
        // already exists, there's a chance that we can't insert back previous
        // name. But it should be fine generally because users are unlikely to
        // expose a variable more than once and calls to expose() are unlikely
        // to contend heavily.

        // remove previous pointer from the map if needed.
        hide();

        // Build the name.
        _name.clear();
        _name.reserve((prefix.size() + name.size()) * 5 / 4);
        if (!prefix.empty()) {
            to_underscored_name(&_name, prefix);
            if (!_name.empty() && BackChar(_name) != '_') {
                _name.push_back('_');
            }
        }
        to_underscored_name(&_name, name);

        VarMapWithLock &m = get_var_map(_name);
        {
            std::unique_lock l(m.mutex);
            auto it = m.find(_name);
            if (it == m.end()) {
                VarEntry entry;
                entry.var = this;
                entry.display_filter = display_filter;
                m[_name] = entry;
                return 0;
            }
        }

        TURBO_RAW_CHECK(get_flag(FLAGS_var_abort_on_same_name) == false, "Abort due to name conflict");
        if (!s_var_may_abort) {
            s_var_may_abort = true;
        }

        TURBO_RAW_LOG(ERROR, "Already exposed `%s' whose value is `%s'",
                      _name.c_str(), describe_exposed(_name).c_str());
        _name.clear();
        return -1;
    }

    bool Variable::is_hidden() const {
        return _name.empty();
    }

    bool Variable::hide() {
        if (_name.empty()) {
            return false;
        }
        VarMapWithLock &m = get_var_map(_name);
        std::unique_lock l(m.mutex);
        auto it = m.find(_name);
        if (it != m.end()) {
            TURBO_RAW_CHECK(1UL == m.erase(_name), "erase bad count");
        } else {
            TURBO_RAW_CHECK(false, "must exist");
        }
        _name.clear();
        return true;
    }

    void Variable::list_exposed(std::vector<std::string> *names,
                                DisplayFilter display_filter) {
        if (names == nullptr) {
            return;
        }
        names->clear();
        if (names->capacity() < 32) {
            names->reserve(count_exposed());
        }
        VarMapWithLock *var_maps = get_var_maps();
        for (size_t i = 0; i < SUB_MAP_COUNT; ++i) {
            VarMapWithLock &m = var_maps[i];
            std::unique_lock mu(m.mutex);
            for (VarMap::const_iterator it = m.begin(); it != m.end(); ++it) {
                if (it->second.display_filter & display_filter) {
                    names->push_back(it->first);
                }
            }
        }
    }

    size_t Variable::count_exposed() {
        size_t n = 0;
        VarMapWithLock *var_maps = get_var_maps();
        for (size_t i = 0; i < SUB_MAP_COUNT; ++i) {
            n += var_maps[i].size();
        }
        return n;
    }

    int Variable::describe_exposed(const std::string_view &name, std::ostream &os,
                                   bool quote_string,
                                   DisplayFilter display_filter) {
        VarMapWithLock &m = get_var_map(name);
        std::unique_lock l(m.mutex);
        auto it = m.find(name);
        if (it == m.end()) {
            return -1;
        }
        if (!(display_filter & it->second.display_filter)) {
            return -1;
        }
        it->second.var->describe(os, quote_string);
        return 0;
    }

    std::string Variable::describe_exposed(const std::string &name,
                                           bool quote_string,
                                           DisplayFilter display_filter) {
        std::ostringstream oss;
        if (describe_exposed(name, oss, quote_string, display_filter) == 0) {
            return oss.str();
        }
        return std::string();
    }

    std::string Variable::get_description() const {
        std::ostringstream os;
        describe(os, false);
        return os.str();
    }

    int Variable::describe_series_exposed(const std::string &name,
                                          std::ostream &os,
                                          const SeriesOptions &options) {
        VarMapWithLock &m = get_var_map(name);
        std::unique_lock l(m.mutex);
        auto it = m.find(name);
        if (it == m.end()) {
            return -1;
        }
        return it->second.var->describe_series(os, options);
    }

    // Underlying buffer to store logs. Comparing to using std::ostringstream
    // directly, this utility exposes more low-level methods so that we avoid
    // creation of std::string which allocates memory internally.
    class CharArrayStreamBuf : public std::streambuf {
    public:
        explicit CharArrayStreamBuf() : _data(nullptr), _size(0) {}

        ~CharArrayStreamBuf();

        int overflow(int ch) override;

        int sync() override;

        void reset();

        std::string data() {
            return std::string(pbase(), pptr() - pbase());
        }

    private:
        char *_data;
        size_t _size;
    };

    CharArrayStreamBuf::~CharArrayStreamBuf() {
        free(_data);
    }

    int CharArrayStreamBuf::overflow(int ch) {
        if (ch == std::streambuf::traits_type::eof()) {
            return ch;
        }
        size_t new_size = std::max(_size * 3 / 2, (size_t) 64);
        char *new_data = (char *) malloc(new_size);
        if (TURBO_UNLIKELY(new_data == nullptr)) {
            setp(nullptr, nullptr);
            return std::streambuf::traits_type::eof();
        }
        memcpy(new_data, _data, _size);
        free(_data);
        _data = new_data;
        const size_t old_size = _size;
        _size = new_size;
        setp(_data, _data + new_size);
        pbump(old_size);
        // if size == 1, this function will call overflow again.
        return sputc(ch);
    }

    int CharArrayStreamBuf::sync() {
        // data are already there.
        return 0;
    }

    void CharArrayStreamBuf::reset() {
        setp(_data, _data + _size);
    }


    // Written by Jack Handy
    // <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
    inline bool wildcmp(const char *wild, const char *str, char question_mark) {
        const char *cp = nullptr;
        const char *mp = nullptr;

        while (*str && *wild != '*') {
            if (*wild != *str && *wild != question_mark) {
                return false;
            }
            ++wild;
            ++str;
        }

        while (*str) {
            if (*wild == '*') {
                if (!*++wild) {
                    return true;
                }
                mp = wild;
                cp = str + 1;
            } else if (*wild == *str || *wild == question_mark) {
                ++wild;
                ++str;
            } else {
                wild = mp;
                str = cp++;
            }
        }

        while (*wild == '*') {
            ++wild;
        }
        return !*wild;
    }

    class WildcardMatcher {
    public:
        WildcardMatcher(const std::string &wildcards,
                        char question_mark,
                        bool on_both_empty)
                : _question_mark(question_mark), _on_both_empty(on_both_empty) {
            if (wildcards.empty()) {
                return;
            }
            std::string name;
            const char wc_pattern[3] = {'*', question_mark, '\0'};
            std::vector<std::string_view> wcs = turbo::str_split(wildcards, by_any_char(",;"));

            for (auto it = wcs.begin(); it != wcs.end(); ++it) {
                name.assign(it->data(), it->size());
                if (name.find_first_of(wc_pattern) != std::string::npos) {
                    if (_wcs.empty()) {
                        _wcs.reserve(8);
                    }
                    _wcs.push_back(name);
                } else {
                    _exact.insert(name);
                }
            }
        }

        bool match(const std::string &name) const {
            if (!_exact.empty()) {
                if (_exact.find(name) != _exact.end()) {
                    return true;
                }
            } else if (_wcs.empty()) {
                return _on_both_empty;
            }
            for (size_t i = 0; i < _wcs.size(); ++i) {
                if (wildcmp(_wcs[i].c_str(), name.c_str(), _question_mark)) {
                    return true;
                }
            }
            return false;
        }

        const std::vector<std::string> &wildcards() const { return _wcs; }

        const std::set<std::string> &exact_names() const { return _exact; }

    private:
        char _question_mark;
        bool _on_both_empty;
        std::vector<std::string> _wcs;
        std::set<std::string> _exact;
    };

    DumpOptions::DumpOptions()
            : quote_string(true), question_mark('?'), display_filter(DISPLAY_ON_PLAIN_TEXT) {}

    int Variable::dump_exposed(Dumper *dumper, const DumpOptions *poptions) {
        if (nullptr == dumper) {
            TURBO_RAW_LOG(ERROR, "Parameter[dumper] is nullptr");
            return -1;
        }
        DumpOptions opt;
        if (poptions) {
            opt = *poptions;
        }
        CharArrayStreamBuf streambuf;
        std::ostream os(&streambuf);
        int count = 0;
        WildcardMatcher black_matcher(opt.black_wildcards,
                                      opt.question_mark,
                                      false);
        WildcardMatcher white_matcher(opt.white_wildcards,
                                      opt.question_mark,
                                      true);

        std::ostringstream dumpped_info;
        const bool log_dummped = get_flag(FLAGS_var_log_dumpped);

        if (white_matcher.wildcards().empty() &&
            !white_matcher.exact_names().empty()) {
            for (std::set<std::string>::const_iterator
                         it = white_matcher.exact_names().begin();
                 it != white_matcher.exact_names().end(); ++it) {
                const std::string &name = *it;
                if (!black_matcher.match(name)) {
                    if (turbo::Variable::describe_exposed(
                            name, os, opt.quote_string, opt.display_filter) != 0) {
                        continue;
                    }
                    if (log_dummped) {
                        dumpped_info << '\n' << name << ": " << streambuf.data();
                    }
                    if (!dumper->dump(name, streambuf.data())) {
                        return -1;
                    }
                    streambuf.reset();
                    ++count;
                }
            }
        } else {
            // Have to iterate all variables.
            std::vector<std::string> varnames;
            turbo::Variable::list_exposed(&varnames, opt.display_filter);
            // Sort the names to make them more readable.
            std::sort(varnames.begin(), varnames.end());
            for (std::vector<std::string>::const_iterator
                         it = varnames.begin(); it != varnames.end(); ++it) {
                const std::string &name = *it;
                if (white_matcher.match(name) && !black_matcher.match(name)) {
                    if (turbo::Variable::describe_exposed(
                            name, os, opt.quote_string, opt.display_filter) != 0) {
                        continue;
                    }
                    if (log_dummped) {
                        dumpped_info << '\n' << name << ": " << streambuf.data();
                    }
                    if (!dumper->dump(name, streambuf.data())) {
                        return -1;
                    }
                    streambuf.reset();
                    ++count;
                }
            }
        }
        if (log_dummped) {
            TURBO_RAW_LOG(INFO, "Dumpped variables:%s", dumpped_info.str().c_str());
        }
        return count;
    }


    // ============= export to files ==============

    std::string read_command_name() {
        std::ifstream fin("/proc/self/stat");
        if (!fin.is_open()) {
            return std::string();
        }
        int pid = 0;
        std::string command_name;
        fin >> pid >> command_name;
        if (!fin.good()) {
            return std::string();
        }
        // Although the man page says the command name is in parenthesis, for
        // safety we normalize the name.
        std::string s;
        if (command_name.size() >= 2UL && command_name[0] == '(' && BackChar(command_name) == ')') {
            // remove parenthesis.
            to_underscored_name(&s,
                                std::string(command_name.data() + 1,
                                            command_name.size() - 2UL));
        } else {
            to_underscored_name(&s, command_name);
        }
        return s;
    }

    class FileDumper : public Dumper {
    public:
        FileDumper(const std::string_view &filename, std::string_view s/*prefix*/)
                : _filename(filename), _fp(nullptr) {
            // setting prefix.
            // remove trailing spaces.
            const char *p = s.data() + s.size();
            for (; p != s.data() && isspace(p[-1]); --p) {}
            s.remove_suffix(s.data() + s.size() - p);
            // normalize it.
            if (!s.empty()) {
                to_underscored_name(&_prefix, s);
                if (BackChar(_prefix) != '_') {
                    _prefix.push_back('_');
                }
            }
        }

        ~FileDumper() {
            close();
        }

        void close() {
            if (_fp) {
                fclose(_fp);
                _fp = nullptr;
            }
        }

    protected:
        bool dump_impl(const std::string_view &name, const std::string_view &desc, const std::string_view &separator) {
            if (_fp == nullptr) {
                turbo::filesystem::path p(_filename);
                auto dir = p.parent_path();
                std::error_code ec;
                if (!turbo::filesystem::exists(dir, ec)) {
                    turbo::filesystem::create_directories(dir, ec);
                    if (ec) {
                        TURBO_RAW_LOG(ERROR, "Fail to create directory=`%s', %s", dir.c_str(), ec.message().c_str());
                        return false;
                    }
                }
                _fp = fopen(_filename.c_str(), "w");
                TURBO_RAW_LOG(INFO, "Open `%s' for dumping", _filename.c_str());
                if (nullptr == _fp) {
                    TURBO_RAW_LOG(ERROR, "Fail to open `%s'", _filename.c_str());
                    return false;
                }
            }
            if (::fprintf(_fp, "%.*s%.*s %.*s %.*s\r\n",
                          (int) _prefix.size(), _prefix.data(),
                          (int) name.size(), name.data(),
                          (int) separator.size(), separator.data(),
                          (int) desc.size(), desc.data()) < 0) {
                TURBO_RAW_LOG(ERROR, "Fail to write into `%s'", _filename.c_str());
                return false;
            }
            return true;
        }

    private:
        std::string _filename;
        FILE *_fp;
        std::string _prefix;
    };

    class CommonFileDumper : public FileDumper {
    public:
        CommonFileDumper(const std::string_view &filename, std::string_view prefix)
                : FileDumper(filename, prefix), _separator(":") {}

        bool dump(const std::string &name, const std::string_view &desc) {
            return dump_impl(name, desc, _separator);
        }

    private:
        std::string _separator;
    };

    class PrometheusFileDumper : public FileDumper {
    public:
        PrometheusFileDumper(const std::string_view &filename, std::string_view prefix)
                : FileDumper(filename, prefix), _separator(" ") {}

        bool dump(const std::string &name, const std::string &desc) {
            return dump_impl(name, desc, _separator);
        }

    private:
        std::string _separator;
    };

    class FileDumperGroup : public Dumper {
    public:
        FileDumperGroup(std::string_view tabs, std::string_view filename,
                        std::string_view s/*prefix*/) {
            turbo::filesystem::path p(filename);
            turbo::filesystem::path base_path = p;
            TURBO_RAW_LOG(INFO, "Dumping to `%s'", p.c_str());
            if (p.extension() == ".data") {
                // .data will be appended later
                base_path = p.parent_path() / p.stem();
            }
            TURBO_RAW_LOG(INFO, "Dumping to `%s'", base_path.c_str());

            std::vector<std::string_view> tgl = turbo::str_split(tabs, ";", turbo::skip_empty());
            for (auto &tg: tgl) {
                std::vector<std::string_view> groups = turbo::str_split(tg, "=", turbo::skip_empty());
                if (groups.size() != 2) {
                    TURBO_RAW_LOG(ERROR, "Invalid tab `%s'", tg.data());
                    continue;
                }
                auto npath = base_path;
                std::string key(groups[0].data(), groups[0].size());
                std::string value(groups[1].data(), groups[1].size());
                TURBO_RAW_LOG(INFO, "Dumping tab `%s' to `%s'", key.c_str(), value.c_str());
                FileDumper *f = new CommonFileDumper(
                        npath.concat(".").concat(key).concat(".data").string(), s);
                WildcardMatcher *m = new WildcardMatcher(value, '?', true);
                dumpers.emplace_back(f, m);
            }
            dumpers.emplace_back(
                    new CommonFileDumper(
                            base_path.concat(".data").string(), s),
                    (WildcardMatcher *) nullptr);
        }

        ~FileDumperGroup() {
            for (size_t i = 0; i < dumpers.size(); ++i) {
                delete dumpers[i].first;
                delete dumpers[i].second;
            }
            dumpers.clear();
        }

        bool dump(const std::string &name, const std::string_view &desc) override {
            for (size_t i = 0; i < dumpers.size() - 1; ++i) {
                if (dumpers[i].second->match(name)) {
                    return dumpers[i].first->dump(name, desc);
                }
            }
            // dump to default file
            return dumpers.back().first->dump(name, desc);
        }

    private:
        std::vector<std::pair<FileDumper *, WildcardMatcher *> > dumpers;
    };

    static pthread_once_t dumping_thread_once = PTHREAD_ONCE_INIT;
    static bool created_dumping_thread = false;
    static pthread_mutex_t dump_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t dump_cond = PTHREAD_COND_INITIALIZER;

    static void *dumping_thread(void *);

    static void launch_dumping_thread() {
        pthread_t thread_id;
        int rc = pthread_create(&thread_id, nullptr, dumping_thread, nullptr);
        if (rc != 0) {
            TURBO_RAW_LOG(FATAL, "Fail to launch dumping thread: %s", terror());
            return;
        }
        // Detach the thread because no one would join it.
        TURBO_RAW_CHECK(0 == pthread_detach(thread_id), "Fail to detach dumping thread");
        created_dumping_thread = true;
    }


    // Start dumping_thread for only once.
    static bool enable_dumping_thread() {
        pthread_once(&dumping_thread_once, launch_dumping_thread);
        return created_dumping_thread;
    }

    static void wakeup_dumping_thread() {
        pthread_cond_signal(&dump_cond);
    }

}  // namespace turbo

TURBO_FLAG(bool, var_dump, false,
           "Create a background thread dumping all var periodically, "
           "all var_dump_* flags are not effective when this flag is off").on_update([]() {
    if (turbo::get_flag(FLAGS_var_dump)) {
        turbo::enable_dumping_thread();
    }
});

TURBO_FLAG(turbo::Duration, var_dump_interval, turbo::Duration::seconds(10),
           "Seconds between consecutive dump").on_update([]() {
    auto interval = turbo::get_flag(FLAGS_var_dump_interval);
    if (interval < turbo::Duration::seconds(10)) {
        TURBO_RAW_LOG(WARNING, "var_dump_interval is too small, "
                               "it may cause high CPU usage");
        turbo::set_flag(&FLAGS_var_dump_interval, turbo::Duration::seconds(10));
    }
});

TURBO_FLAG(std::string, var_dump_file, "monitor/var.<app>.data",
           "Dump var into this file").on_update(turbo::wakeup_dumping_thread);

TURBO_FLAG(std::string, var_dump_include, "",
           "Dump var matching these wildcards, "
           "separated by semicolon(;), empty means including all").on_update(turbo::wakeup_dumping_thread);

TURBO_FLAG(std::string, var_dump_exclude, "",
           "Dump var excluded from these wildcards, "
           "separated by semicolon(;), empty means no exclusion").on_update(turbo::wakeup_dumping_thread);

TURBO_FLAG(std::string, var_dump_prefix, "<app>",
           "Every dumped name starts with this prefix").on_update(turbo::wakeup_dumping_thread);

TURBO_FLAG(std::string, var_dump_tabs,
           "latency=*_latency*"
           ";qps=*_qps*"
           ";error=*_error*"
           ";system=*process_*,*malloc_*,*kernel_*",
           "Dump var into different tabs according to the filters (seperated by semicolon), "
           "format: *(tab_name=wildcards;)").on_update(turbo::wakeup_dumping_thread);

namespace turbo {

    void *dumping_thread(void *) {
        // NOTE: this variable was declared as static <= r34381, which was
        // destructed when program exits and caused coredumps.
        PlatformThread::set_name("var_dumper");
        const std::string command_name = read_command_name();
        std::string last_filename;
        while (1) {
            // We can't access string flags directly because it's thread-unsafe.
            std::string filename = get_flag(FLAGS_var_dump_file);
            DumpOptions options;
            options.white_wildcards = get_flag(FLAGS_var_dump_include);
            options.black_wildcards = get_flag(FLAGS_var_dump_exclude);
            std::string prefix = get_flag(FLAGS_var_dump_prefix);
            std::string tabs = get_flag(FLAGS_var_dump_tabs);

            if (get_flag(FLAGS_var_dump) && !filename.empty()) {
                // Replace first <app> in filename with program name. We can't use
                // pid because a same binary should write the data to the same
                // place, otherwise restarting of app may confuse noah with a lot
                // of *.data. noah takes 1.5 days to figure out that some data is
                // outdated and to be removed.
                const size_t pos = filename.find("<app>");
                if (pos != std::string::npos) {
                    filename.replace(pos, 5/*<app>*/, command_name);
                }
                if (last_filename != filename) {
                    last_filename = filename;
                    TURBO_RAW_LOG(INFO, "Write all var to %s every %s.",
                                  filename.c_str(), get_flag(FLAGS_var_dump_interval).to_string().c_str());
                }
                const size_t pos2 = prefix.find("<app>");
                if (pos2 != std::string::npos) {
                    prefix.replace(pos2, 5/*<app>*/, command_name);
                }
                FileDumperGroup dumper(tabs, filename, prefix);
                int nline = Variable::dump_exposed(&dumper, &options);
                if (nline < 0) {
                    TURBO_RAW_LOG(ERROR, "Fail to dump vars into %s", filename.c_str());
                }
            }


            // We need to separate the sleeping into a long interruptible sleep
            // and a short uninterruptible sleep. Doing this because we wake up
            // this thread in gflag validators. If this thread dumps just after
            // waking up from the condition, the gflags may not even be updated.
            const Duration post_sleep = turbo::Duration::milliseconds(50);
            auto cond_sleep = get_flag(FLAGS_var_dump_interval) - post_sleep;
            if (cond_sleep < turbo::Duration::zero()) {
                TURBO_RAW_LOG(ERROR, "Bad cond_sleep %s", cond_sleep.to_string().c_str());
                cond_sleep = get_flag(FLAGS_var_dump_interval);
            }
            timespec deadline = from_now(cond_sleep).to_timespec();
            pthread_mutex_lock(&dump_mutex);
            pthread_cond_timedwait(&dump_cond, &dump_mutex, &deadline);
            pthread_mutex_unlock(&dump_mutex);
            turbo::sleep_for(post_sleep);
        }
    }

    void to_underscored_name(std::string *name, const std::string_view &src) {
        name->reserve(name->size() + src.size() + 8/*just guess*/);
        for (const char *p = src.data(); p != src.data() + src.size(); ++p) {
            if (isalpha(*p)) {
                if (*p < 'a') { // upper cases
                    if (p != src.data() && !isupper(p[-1]) &&
                        BackChar(*name) != '_') {
                        name->push_back('_');
                    }
                    name->push_back(*p - 'A' + 'a');
                } else {
                    name->push_back(*p);
                }
            } else if (isdigit(*p)) {
                name->push_back(*p);
            } else if (name->empty() || BackChar(*name) != '_') {
                name->push_back('_');
            }
        }
    }

}  // namespace turbo
