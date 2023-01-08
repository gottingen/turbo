
#include <pthread.h>
#include <set>                                  // std::set
#include <fstream>                              // std::ifstream
#include <sstream>                              // std::ostringstream
#include <gflags/gflags.h>
#include "flare/files/filesystem.h"
#include "flare/container/flat_map.h"           // flare::container::FlatMap
#include "flare/base/scoped_lock.h"                   // FLARE_SCOPE_LOCK
#include "flare/strings/string_splitter.h"               // flare::StringSplitter
#include "flare/base/errno.h"                          // flare_error
#include "flare/times/time.h"                          // milliseconds_from_now
#include "flare/metrics/gflag.h"
#include "flare/metrics/variable_base.h"
#include "flare/strings/utility.h"
#include "flare/strings/str_format.h"
#include "flare/strings/ends_with.h"
#include "flare/strings/strip.h"
#include "flare/strings/str_join.h"
#include "flare/log/logging.h"

namespace flare {

    DEFINE_bool(save_series, true,
                "Save values of last 60 seconds, last 60 minutes,"
                " last 24 hours and last 30 days for ploting");

    DEFINE_bool(quote_vector, true,
                "Quote description of Vector<> to make it valid to noah");

    DEFINE_bool(variable_abort_on_same_name, false,
                "Abort when names of variable are same");
    // Remember abort request before variable_abort_on_same_name is initialized.
    static bool s_var_may_abort = false;

    static bool validate_variable_abort_on_same_name(const char *, bool v) {
        if (v && s_var_may_abort) {
            // Name conflict happens before handling args of main(), this is
            // generally caused by global variable.
            FLARE_LOG(FATAL) << "Abort due to name conflict";
            abort();
        }
        return true;
    }

    const bool FLARE_ALLOW_UNUSED dummy_variable_abort_on_same_name = ::google::RegisterFlagValidator(
            &FLAGS_variable_abort_on_same_name, validate_variable_abort_on_same_name);


    DEFINE_bool(variable_log_dumpped, false,
                "[For debugging] print dumpped info"
                " into logstream before call Dumpper");

    const size_t SUB_MAP_COUNT = 32;  // must be power of 2
    static_assert(!(SUB_MAP_COUNT & (SUB_MAP_COUNT - 1)), "must be power of 2");

    class VarEntry {
    public:
        VarEntry() : var(nullptr), filter(DISPLAY_ON_ALL) {}

        variable_base *var;
        display_filter filter;
    };

    typedef flare::container::FlatMap<std::string, VarEntry> VarMap;

    struct VarMapWithLock : public VarMap {
        pthread_mutex_t mutex;

        VarMapWithLock() {
            FLARE_CHECK_EQ(0, init(1024, 80));
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&mutex, &attr);
            pthread_mutexattr_destroy(&attr);
        }
    };

    // We have to initialize global map on need because variable is possibly used
    // before main().
    static pthread_once_t s_var_maps_once = PTHREAD_ONCE_INIT;
    static VarMapWithLock *s_var_maps = nullptr;

    static void init_var_maps() {
        // It's probably slow to initialize all sub maps, but rpc often expose
        // variables before user. So this should not be an issue to users.
        s_var_maps = new VarMapWithLock[SUB_MAP_COUNT];
    }

    inline size_t sub_map_index(const std::string &str) {
        if (str.empty()) {
            return 0;
        }
        size_t h = 0;
        // we're assume that str is ended with '\0', which may not be in general
        for (const char *p = str.c_str(); *p; ++p) {
            h = h * 5 + *p;
        }
        return h & (SUB_MAP_COUNT - 1);
    }

    inline VarMapWithLock *get_var_maps() {
        pthread_once(&s_var_maps_once, init_var_maps);
        return s_var_maps;
    }

    inline VarMapWithLock &get_var_map(const std::string &name) {
        VarMapWithLock &m = get_var_maps()[sub_map_index(name)];
        return m;
    }

    variable_base::~variable_base() {
        FLARE_CHECK(!hide()) << "Subclass of variable_base MUST call hide() manually in their"
                                " dtors to avoid displaying a variable that is just destructing";
    }

    int variable_base::expose_impl(const std::string_view &prefix,
                                   const std::string_view &name,
                                   const std::string_view &help,
                                   const tag_type &tags,
                                   display_filter filter) {
        if (name.empty()) {
            FLARE_LOG(ERROR) << "Parameter[name] is empty";
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
        _index_name.clear();
        _tags.clear();
        _tags = tags;
        std::string tags_str = flare::string_join(_tags, "_", flare::pair_formatter("_"));
        _name.reserve((prefix.size() + name.size()) * 5 / 4);
        _index_name.reserve((prefix.size() + name.size() + tags_str.size()) * 5 / 4);
        if (!prefix.empty()) {
            to_underscored_name(&_name, prefix);
            if (!_name.empty() && flare::back_char(_name) != '_') {
                _name.push_back('_');
            }
        }
        to_underscored_name(&_name, name);
        _index_name = _name;
        if (_tags.size() > 0) {
            _index_name.push_back('_');
            to_underscored_name(&_index_name, tags_str);
        }
        if (help.empty()) {
            _help = _name;
        } else {
            _help.assign(help.data(), help.size());
        }
        VarMapWithLock &m = get_var_map(_index_name);
        {
            FLARE_SCOPED_LOCK(m.mutex);
            VarEntry *entry = m.seek(_index_name);
            if (entry == nullptr) {
                entry = &m[_index_name];
                entry->var = this;
                entry->filter = filter;
                return 0;
            }
        }
        if (FLAGS_variable_abort_on_same_name) {
            FLARE_LOG(FATAL) << "Abort due to name conflict";
            abort();
        } else if (!s_var_may_abort) {
            // Mark name conflict occurs, If this conflict happens before
            // initialization of variable_abort_on_same_name, the validator will
            // abort the program if needed.
            s_var_may_abort = true;
        }

        FLARE_LOG(ERROR) << "Already exposed `" << _index_name << "' whose value is `"
                         << describe_exposed(_index_name) << '\'';
        _name.clear();
        _index_name.clear();
        return -1;
    }

    bool variable_base::hide() {
        if (_index_name.empty()) {
            return false;
        }
        VarMapWithLock &m = get_var_map(_index_name);
        FLARE_SCOPED_LOCK(m.mutex);
        VarEntry *entry = m.seek(_index_name);
        if (entry) {
            FLARE_CHECK_EQ(1UL, m.erase(_index_name));
        } else {
            FLARE_CHECK(false) << "`" << _index_name << "' must exist";
        }
        _index_name.clear();
        _name.clear();
        return true;
    }

    void variable_base::copy_metric_family(cache_metrics &metric) const {
        metric.name = _name;
        metric.tags = _tags;
        metric.help = _help;
    }

    void variable_base::list_exposed(std::vector<std::string> *names,
                                     display_filter filter) {
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
            std::unique_lock<pthread_mutex_t> mu(m.mutex);
            size_t n = 0;
            for (VarMap::const_iterator it = m.begin(); it != m.end(); ++it) {
                if (++n >= 256/*max iterated one pass*/) {
                    VarMap::PositionHint hint;
                    m.save_iterator(it, &hint);
                    n = 0;
                    mu.unlock();  // yield
                    mu.lock();
                    it = m.restore_iterator(hint);
                    if (it == m.begin()) { // resized
                        names->clear();
                    }
                    if (it == m.end()) {
                        break;
                    }
                }
                if (it->second.filter & filter) {
                    names->push_back(it->first);
                }
            }
        }
    }

    void variable_base::list_metrics(std::vector<cache_metrics> *metrics) {
        if (metrics == nullptr) {
            return;
        }
        metrics->clear();
        if (metrics->capacity() < 32) {
            metrics->reserve(count_exposed());
        }
        VarMapWithLock *var_maps = get_var_maps();
        for (size_t i = 0; i < SUB_MAP_COUNT; ++i) {
            VarMapWithLock &m = var_maps[i];
            std::unique_lock<pthread_mutex_t> mu(m.mutex);
            size_t n = 0;
            for (VarMap::const_iterator it = m.begin(); it != m.end(); ++it) {
                if (++n >= 256/*max iterated one pass*/) {
                    VarMap::PositionHint hint;
                    m.save_iterator(it, &hint);
                    n = 0;
                    mu.unlock();  // yield
                    mu.lock();
                    it = m.restore_iterator(hint);
                    if (it == m.begin()) { // resized
                        metrics->clear();
                    }
                    if (it == m.end()) {
                        break;
                    }
                }
                FLARE_LOG(INFO) << it->first << ": " << it->second.filter;
                if (it->second.filter & DISPLAY_ON_METRICS) {
                    cache_metrics m;
                    it->second.var->collect_metrics(m);
                    metrics->push_back(std::move(m));
                }
            }
        }
    }

    size_t variable_base::count_exposed() {
        size_t n = 0;
        VarMapWithLock *var_maps = get_var_maps();
        for (size_t i = 0; i < SUB_MAP_COUNT; ++i) {
            n += var_maps[i].size();
        }
        return n;
    }

    int variable_base::describe_exposed(const std::string &name, std::ostream &os,
                                        bool quote_string,
                                        display_filter filter) {
        VarMapWithLock &m = get_var_map(name);
        FLARE_SCOPED_LOCK(m.mutex);
        VarEntry *p = m.seek(name);
        if (p == nullptr) {
            return -1;
        }
        if (!(filter & p->filter)) {
            return -1;
        }
        p->var->describe(os, quote_string);
        return 0;
    }

    std::string variable_base::describe_exposed(const std::string &name,
                                                bool quote_string,
                                                display_filter filter) {
        std::ostringstream oss;
        if (describe_exposed(name, oss, quote_string, filter) == 0) {
            return oss.str();
        }
        return std::string();
    }

    std::string variable_base::get_description() const {
        std::ostringstream os;
        describe(os, false);
        return os.str();
    }


    int variable_base::describe_series_exposed(const std::string &name,
                                               std::ostream &os,
                                               const variable_series_options &options) {
        VarMapWithLock &m = get_var_map(name);
        FLARE_SCOPED_LOCK(m.mutex);
        VarEntry *p = m.seek(name);
        if (p == nullptr) {
            return -1;
        }
        return p->var->describe_series(os, options);
    }

    // TODO(gejun): This is copied from otherwhere, common it if possible.

    // Underlying buffer to store logs. Comparing to using std::ostringstream
    // directly, this utility exposes more low-level methods so that we avoid
    // creation of std::string which allocates memory internally.
    class char_array_streambuf : public std::streambuf {
    public:
        explicit char_array_streambuf() : _data(nullptr), _size(0) {}

        ~char_array_streambuf();

        int overflow(int ch) override;

        int sync() override;

        void reset();

        std::string_view data() {
            return std::string_view(pbase(), pptr() - pbase());
        }

    private:
        char *_data;
        size_t _size;
    };

    char_array_streambuf::~char_array_streambuf() {
        free(_data);
    }

    int char_array_streambuf::overflow(int ch) {
        if (ch == std::streambuf::traits_type::eof()) {
            return ch;
        }
        size_t new_size = std::max(_size * 3 / 2, (size_t) 64);
        char *new_data = (char *) malloc(new_size);
        if (FLARE_UNLIKELY(new_data == nullptr)) {
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

    int char_array_streambuf::sync() {
        // data are already there.
        return 0;
    }

    void char_array_streambuf::reset() {
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
            for (flare::StringMultiSplitter sp(wildcards.c_str(), ",;");
                 sp != nullptr; ++sp) {
                name.assign(sp.field(), sp.length());
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

    variable_dump_options::variable_dump_options()
            : quote_string(true), question_mark('?'), filter(DISPLAY_ON_PLAIN_TEXT) {}

    int variable_base::dump_exposed(variable_dumper *dumper, const variable_dump_options *poptions) {
        if (nullptr == dumper) {
            FLARE_LOG(ERROR) << "Parameter[dumper] is nullptr";
            return -1;
        }
        variable_dump_options opt;
        if (poptions) {
            opt = *poptions;
        }
        char_array_streambuf streambuf;
        std::ostream os(&streambuf);
        int count = 0;
        WildcardMatcher black_matcher(opt.black_wildcards,
                                      opt.question_mark,
                                      false);
        WildcardMatcher white_matcher(opt.white_wildcards,
                                      opt.question_mark,
                                      true);

        std::ostringstream dumpped_info;
        const bool log_dummped = FLAGS_variable_log_dumpped;

        if (white_matcher.wildcards().empty() &&
            !white_matcher.exact_names().empty()) {
            for (std::set<std::string>::const_iterator
                         it = white_matcher.exact_names().begin();
                 it != white_matcher.exact_names().end(); ++it) {
                const std::string &name = *it;
                if (!black_matcher.match(name)) {
                    if (flare::variable_base::describe_exposed(
                            name, os, opt.quote_string, opt.filter) != 0) {
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
            flare::variable_base::list_exposed(&varnames, opt.filter);
            // Sort the names to make them more readable.
            std::sort(varnames.begin(), varnames.end());
            for (std::vector<std::string>::const_iterator
                         it = varnames.begin(); it != varnames.end(); ++it) {
                const std::string &name = *it;
                if (white_matcher.match(name) && !black_matcher.match(name)) {
                    if (flare::variable_base::describe_exposed(
                            name, os, opt.quote_string, opt.filter) != 0) {
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
            FLARE_LOG(INFO) << "Dumpped variables:" << dumpped_info.str();
        }
        return count;
    }

    int variable_base::dump_metrics(metrics_dumper *dumper, const metrics_dump_options *poptions) {
        if (nullptr == dumper) {
            FLARE_LOG(ERROR) << "Parameter[dumper] is nullptr";
            return -1;
        }
        metrics_dump_options opt;
        if (poptions) {
            opt = *poptions;
        }
        int count = 0;
        WildcardMatcher black_matcher(opt.black_wildcards,
                                      opt.question_mark,
                                      false);
        WildcardMatcher white_matcher(opt.white_wildcards,
                                      opt.question_mark,
                                      true);

        std::ostringstream dumpped_info;
        const bool log_dummped = FLAGS_variable_log_dumpped;
        std::vector<cache_metrics> cm;
        flare::variable_base::list_metrics(&cm);
        for (auto it = cm.begin(); it != cm.end(); ++it) {
            const std::string &name = it->name;
            if (white_matcher.match(name) && !black_matcher.match(name)) {
                if (!dumper->dump(*it, opt.dump_time)) {
                    return -1;
                }
                ++count;
            }
        }
        if (log_dummped) {
            FLARE_LOG(INFO) << "Dumpped variables:" << dumpped_info.str();
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
        if (command_name.size() >= 2UL && command_name[0] == '(' &&
            flare::back_char(command_name) == ')') {
            // remove parenthesis.
            to_underscored_name(&s,
                                std::string_view(command_name.data() + 1,
                                                 command_name.size() - 2UL));
        } else {
            to_underscored_name(&s, command_name);
        }
        return s;
    }

    class FileDumper : public variable_dumper {
    public:
        FileDumper(const std::string &filename, std::string_view s/*prefix*/)
                : _filename(filename), _fp(nullptr) {
            // setting prefix.
            // remove trailing spaces.
            const char *p = s.data() + s.size();
            for (; p != s.data() && isspace(p[-1]); --p) {}
            s.remove_suffix(s.data() + s.size() - p);
            // normalize it.
            if (!s.empty()) {
                to_underscored_name(&_prefix, s);
                if (flare::back_char(_prefix) != '_') {
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

        bool dump(const std::string &name, const std::string_view &desc) override {
            if (_fp == nullptr) {
                std::error_code ec;
                flare::file_path dirPath = flare::file_path(_filename).parent_path();
                if (!flare::create_directories(dirPath, ec)) {
                    FLARE_LOG(ERROR) << "Fail to create directory=`" << dirPath.c_str()
                                     << "', " << ec.message();
                    return false;
                }
                _fp = fopen(_filename.c_str(), "w");
                if (nullptr == _fp) {
                    FLARE_LOG(ERROR) << "Fail to open " << _filename;
                    return false;
                }
            }
            if (fprintf(_fp, "%.*s%.*s : %.*s\r\n",
                        (int) _prefix.size(), _prefix.data(),
                        (int) name.size(), name.data(),
                        (int) desc.size(), desc.data()) < 0) {
                FLARE_PLOG(ERROR) << "Fail to write into " << _filename;
                return false;
            }
            return true;
        }

    private:

        std::string _filename;
        FILE *_fp;
        std::string _prefix;
    };

    class FileDumperGroup : public variable_dumper {
    public:
        FileDumperGroup(std::string tabs, std::string filename,
                        std::string_view s/*prefix*/) {
            std::string_view path_str(filename);
            if (flare::ends_with(path_str, ".data")) {
                // .data will be appended later
                flare::consume_suffix(&path_str, ".data");
            }

            for (flare::KeyValuePairsSplitter sp(tabs, ';', '='); sp; ++sp) {
                std::string key = flare::as_string(sp.key());
                std::string value = flare::as_string(sp.value());
                std::string pathString(path_str.data(), path_str.size());
                flare::string_appendf(&pathString, ".%s.data", key.c_str());
                FileDumper *f = new FileDumper(pathString, s);
                WildcardMatcher *m = new WildcardMatcher(value, '?', true);
                dumpers.emplace_back(f, m);
            }
            std::string pathString(path_str.data(), path_str.size());
            pathString += ".data";
            dumpers.emplace_back(
                    new FileDumper(pathString, s),
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

    DEFINE_bool(variable_dump, false,
                "Create a background thread dumping all variable periodically, "
                "all variable_dump_* flags are not effective when this flag is off");
    DEFINE_int32(variable_dump_interval, 10, "Seconds between consecutive dump");
    DEFINE_string(variable_dump_file, "monitor/variable.<app>.data", "Dump variable into this file");
    DEFINE_string(variable_dump_include, "", "Dump variable matching these wildcards, "
                                             "separated by semicolon(;), empty means including all");
    DEFINE_string(variable_dump_exclude, "", "Dump variable excluded from these wildcards, "
                                             "separated by semicolon(;), empty means no exclusion");
    DEFINE_string(variable_dump_prefix, "<app>", "Every dumped name starts with this prefix");
    DEFINE_string(variable_dump_tabs, "latency=*_latency*"
                                      "; qps=*_qps*"
                                      "; error=*_error*"
                                      "; system=*process_*,*malloc_*,*kernel_*",
                  "Dump variable into different tabs according to the filters (seperated by semicolon), "
                  "format: *(tab_name=wildcards;)");

#if !defined(UNIT_TEST)
    // Expose variable-releated gflags so that they're collected by noah.
    // Maybe useful when debugging process of monitoring.
    static metrics_gflag s_gflag_variable_dump_interval("variable_dump_interval");
#endif

    // The background thread to export all variable periodically.
    static void *dumping_thread(void *) {
        // NOTE: this variable was declared as static <= r34381, which was
        // destructed when program exits and caused coredumps.
        const std::string command_name = read_command_name();
        std::string last_filename;
        while (1) {
            // We can't access string flags directly because it's thread-unsafe.
            std::string filename;
            variable_dump_options options;
            std::string prefix;
            std::string tabs;
            if (!google::GetCommandLineOption("variable_dump_file", &filename)) {
                FLARE_LOG(ERROR) << "Fail to get gflag variable_dump_file";
                return nullptr;
            }
            if (!google::GetCommandLineOption("variable_dump_include",
                                              &options.white_wildcards)) {
                FLARE_LOG(ERROR) << "Fail to get gflag variable_dump_include";
                return nullptr;
            }
            if (!google::GetCommandLineOption("variable_dump_exclude",
                                              &options.black_wildcards)) {
                FLARE_LOG(ERROR) << "Fail to get gflag variable_dump_exclude";
                return nullptr;
            }
            if (!google::GetCommandLineOption("variable_dump_prefix", &prefix)) {
                FLARE_LOG(ERROR) << "Fail to get gflag variable_dump_prefix";
                return nullptr;
            }
            if (!google::GetCommandLineOption("variable_dump_tabs", &tabs)) {
                FLARE_LOG(ERROR) << "Fail to get gflags variable_dump_tabs";
                return nullptr;
            }

            if (FLAGS_variable_dump && !filename.empty()) {
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
                    FLARE_LOG(INFO) << "Write all variable to " << filename << " every "
                                    << FLAGS_variable_dump_interval << " seconds.";
                }
                const size_t pos2 = prefix.find("<app>");
                if (pos2 != std::string::npos) {
                    prefix.replace(pos2, 5/*<app>*/, command_name);
                }
                FileDumperGroup dumper(tabs, filename, prefix);
                int nline = variable_base::dump_exposed(&dumper, &options);
                if (nline < 0) {
                    FLARE_LOG(ERROR) << "Fail to dump vars into " << filename;
                }
            }

            // We need to separate the sleeping into a long interruptible sleep
            // and a short uninterruptible sleep. Doing this because we wake up
            // this thread in gflag validators. If this thread dumps just after
            // waking up from the condition, the gflags may not even be updated.
            const int post_sleep_ms = 50;
            int cond_sleep_ms = FLAGS_variable_dump_interval * 1000 - post_sleep_ms;
            if (cond_sleep_ms < 0) {
                FLARE_LOG(ERROR) << "Bad cond_sleep_ms=" << cond_sleep_ms;
                cond_sleep_ms = 10000;
            }
            timespec deadline = flare::time_point::future_unix_millis(cond_sleep_ms).to_timespec();
            pthread_mutex_lock(&dump_mutex);
            pthread_cond_timedwait(&dump_cond, &dump_mutex, &deadline);
            pthread_mutex_unlock(&dump_mutex);
            usleep(post_sleep_ms * 1000);
        }
    }

    static void launch_dumping_thread() {
        pthread_t thread_id;
        int rc = pthread_create(&thread_id, nullptr, dumping_thread, nullptr);
        if (rc != 0) {
            FLARE_LOG(FATAL) << "Fail to launch dumping thread: " << flare_error(rc);
            return;
        }
        // Detach the thread because no one would join it.
        FLARE_CHECK_EQ(0, pthread_detach(thread_id));
        created_dumping_thread = true;
    }

// Start dumping_thread for only once.
    static bool enable_dumping_thread() {
        pthread_once(&dumping_thread_once, launch_dumping_thread);
        return created_dumping_thread;
    }

    static bool validate_variable_dump(const char *, bool enabled) {
        if (enabled) {
            return enable_dumping_thread();
        }
        return true;
    }

    const bool FLARE_ALLOW_UNUSED dummy_variable_dump = ::google::RegisterFlagValidator(
            &FLAGS_variable_dump, validate_variable_dump);

    // validators (to make these gflags reloadable in flare)
    static bool validate_variable_dump_interval(const char *, int32_t v) {
        // FIXME: -variable_dump_interval is actually unreloadable but we need to
        // check validity of it, so we still add this validator. In practice
        // this is just fine since people rarely have the intention of modifying
        // this flag at runtime.
        if (v < 1) {
            FLARE_LOG(ERROR) << "Invalid variable_dump_interval=" << v;
            return false;
        }
        return true;
    }

    const bool FLARE_ALLOW_UNUSED dummy_variable_dump_interval = ::google::RegisterFlagValidator(
            &FLAGS_variable_dump_interval, validate_variable_dump_interval);

    static bool validate_variable_log_dumpped(const char *, bool) { return true; }

    const bool FLARE_ALLOW_UNUSED dummy_variable_log_dumpped = ::google::RegisterFlagValidator(
            &FLAGS_variable_log_dumpped, validate_variable_log_dumpped);

    static bool wakeup_dumping_thread(const char *, const std::string &) {
        // We're modifying a flag, wake up dumping_thread to generate
        // a new file soon.
        pthread_cond_signal(&dump_cond);
        return true;
    }

    const bool FLARE_ALLOW_UNUSED dummy_variable_dump_file = ::google::RegisterFlagValidator(
            &FLAGS_variable_dump_file, wakeup_dumping_thread);
    const bool FLARE_ALLOW_UNUSED dummy_variable_dump_filter = ::google::RegisterFlagValidator(
            &FLAGS_variable_dump_include, wakeup_dumping_thread);
    const bool FLARE_ALLOW_UNUSED dummy_variable_dump_exclude = ::google::RegisterFlagValidator(
            &FLAGS_variable_dump_exclude, wakeup_dumping_thread);
    const bool FLARE_ALLOW_UNUSED dummy_variable_dump_prefix = ::google::RegisterFlagValidator(
            &FLAGS_variable_dump_prefix, wakeup_dumping_thread);
    const bool FLARE_ALLOW_UNUSED dummy_variable_dump_tabs = ::google::RegisterFlagValidator(
            &FLAGS_variable_dump_tabs, wakeup_dumping_thread);

    void to_underscored_name(std::string *name, const std::string_view &src) {
        name->reserve(name->size() + src.size() + 8/*just guess*/);
        for (const char *p = src.data(); p != src.data() + src.size(); ++p) {
            if (isalpha(*p)) {
                if (*p < 'a') { // upper cases
                    if (p != src.data() && !isupper(p[-1]) &&
                        flare::back_char(*name) != '_') {
                        name->push_back('_');
                    }
                    name->push_back(*p - 'A' + 'a');
                } else {
                    name->push_back(*p);
                }
            } else if (isdigit(*p)) {
                name->push_back(*p);
            } else if (name->empty() || flare::back_char(*name) != '_') {
                name->push_back('_');
            }
        }
    }

// UT don't need default variables.
#if !defined(UNIT_TEST)
    // Without these, default_variables.o are stripped.
    // At least working in gcc 4.8
    extern int do_link_default_variables;
    int dummy = do_link_default_variables;
#endif

}  // namespace flare
