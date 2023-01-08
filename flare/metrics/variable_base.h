
#ifndef  FLARE_VARIABLE_VARIABLE_BASE_H_
#define  FLARE_VARIABLE_VARIABLE_BASE_H_

#include <ostream>                     // std::ostream
#include <string>                      // std::string
#include <vector>                      // std::vector
#include <string_view>
#include <unordered_map>
#include <gflags/gflags_declare.h>
#include "flare/base/profile.h"               // FLARE_DISALLOW_COPY_AND_ASSIGN
#include "flare/times/time.h"
#include "flare/metrics/cache_metric.h"
#include "flare/metrics/dumper.h"

namespace flare {

    DECLARE_bool(save_series);


    struct variable_series_options {
        variable_series_options() : fixed_length(true), test_only(false) {}

        bool fixed_length; // useless now
        bool test_only;
    };

    // Base class of all variable.
    //
    // About thread-safety:
    //   variable is thread-compatible:
    //     Namely you can create/destroy/expose/hide or do whatever you want to
    //     different variable simultaneously in different threads.
    //   variable is NOT thread-safe:
    //     You should not operate one variable from different threads simultaneously.
    //     If you need to, protect the ops with locks. Similarly with ordinary
    //     variables, const methods are thread-safe, namely you can call
    //     describe()/get_description()/get_value() etc from diferent threads
    //     safely (provided that there's no non-const methods going on).
    class variable_base {
    public:
        typedef std::unordered_map<std::string, std::string> tag_type;
    public:
        variable_base() {}

        virtual ~variable_base();

        // Implement this method to print the variable into ostream.
        virtual void describe(std::ostream &, bool quote_string) const = 0;

        virtual void collect_metrics(cache_metrics &metric) const {}

        // string form of describe().
        std::string get_description() const;


        // Describe saved series as a json-string into the stream.
        // The output will be ploted by flot.js
        // Returns 0 on success, 1 otherwise(this variable does not save series).
        virtual int describe_series(std::ostream &, const variable_series_options &) const { return 1; }

        // Expose this variable globally so that it's counted in following
        // functions:
        //   list_exposed
        //   count_exposed
        //   describe_exposed
        //   find_exposed
        // Return 0 on success, -1 otherwise.
        int expose(const std::string_view &name,
                   const std::string_view &help,
                   const tag_type &tags = tag_type(),
                   display_filter filter = DISPLAY_ON_ALL
        ) {
            return expose_impl(std::string_view(), name, help, tags, filter);
        }

        // Expose this variable with a prefix.
        // Example:
        //   namespace foo {
        //   namespace bar {
        //   class ApplePie {
        //       ApplePie() {
        //           // foo_bar_apple_pie_error
        //           _error.expose_as("foo_bar_apple_pie", "error");
        //       }
        //   private:
        //       flare::counter<int> _error;
        //   };
        //   }  // foo
        //   }  // bar
        // Returns 0 on success, -1 otherwise.
        int expose_as(const std::string_view &prefix,
                      const std::string_view &name,
                      const std::string_view &help,
                      const tag_type &tags = tag_type(),
                      display_filter filter = DISPLAY_ON_ALL
        ) {
            return expose_impl(prefix, name, help, tags, filter);
        }

        // Hide this variable so that it's not counted in *_exposed functions.
        // Returns false if this variable is already hidden.
        // CAUTION!! Subclasses must call hide() manually to avoid displaying
        // a variable that is just destructing.
        bool hide();

        // Get exposed name. If this variable is not exposed, the name is empty.
        const std::string &name() const { return _name; }

        const std::string &index_name() const { return _index_name; }

        const std::string &help() const { return _help; }

        const std::unordered_map<std::string, std::string> &tags() const { return _tags; }

        // ====================================================================

        // Put names of all exposed variables into `names'.
        // If you want to print all variables, you have to go through `names'
        // and call `describe_exposed' on each name. This prevents an iteration
        // from taking the lock too long.
        static void list_exposed(std::vector<std::string> *names,
                                 display_filter = DISPLAY_ON_ALL);

        static void list_metrics(std::vector<cache_metrics> *names);

        // Get number of exposed variables.
        static size_t count_exposed();

        // Find an exposed variable by `name' and put its description into `os'.
        // Returns 0 on found, -1 otherwise.
        static int describe_exposed(const std::string &name,
                                    std::ostream &os,
                                    bool quote_string = false,
                                    display_filter = DISPLAY_ON_ALL);

        // String form. Returns empty string when not found.
        static std::string describe_exposed(const std::string &name,
                                            bool quote_string = false,
                                            display_filter = DISPLAY_ON_ALL);

        // Describe saved series of variable `name' as a json-string into `os'.
        // The output will be ploted by flot.js
        // Returns 0 on success, 1 when the variable does not save series, -1
        // otherwise (no variable named so).
        static int describe_series_exposed(const std::string &name,
                                           std::ostream &,
                                           const variable_series_options &);


        // Find all exposed variables matching `white_wildcards' but
        // `black_wildcards' and send them to `dumper'.
        // Use default options when `options' is nullptr.
        // Return number of dumped variables, -1 on error.
        static int dump_exposed(variable_dumper *dumper, const variable_dump_options *options);

        static int dump_metrics(metrics_dumper *dumper, const metrics_dump_options *options);

        void copy_metric_family(cache_metrics &metric) const;

    protected:

        virtual int expose_impl(const std::string_view &prefix,
                                const std::string_view &name,
                                const std::string_view &help,
                                const tag_type &tags,
                                display_filter filter);


    protected:
        std::string _name;
        std::string _index_name;
        std::string _help;
        tag_type _tags;
        // variable uses TLS, thus copying/assignment need to copy TLS stuff as well,
        // which is heavy. We disable copying/assignment now.
        FLARE_DISALLOW_COPY_AND_ASSIGN(variable_base);
    };

    // Make name only use lowercased alphabets / digits / underscores, and append
    // the result to `out'.
    // Examples:
    //   foo-inl.h       -> foo_inl_h
    //   foo::bar::Apple -> foo_bar_apple
    //   Car_Rot         -> car_rot
    //   FooBar          -> foo_bar
    //   RPCTest         -> rpctest
    //   HELLO           -> hello
    void to_underscored_name(std::string *out, const std::string_view &name);

}  // namespace flare

// Make variables printable.
namespace std {

    inline ostream &operator<<(ostream &os, const ::flare::variable_base &var) {
        var.describe(os, false);
        return os;
    }

}  // namespace std

#endif  // FLARE_VARIABLE_VARIABLE_BASE_H_
