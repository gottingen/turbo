// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// Date: 2014/09/22 11:57:43

#ifndef  BVAR_VARIABLE_H
#define  BVAR_VARIABLE_H

#include <ostream>                     // std::ostream
#include <string>                      // std::string
#include <string_view>
#include <vector>                      // std::vector
#include "turbo/platform/port.h"

namespace turbo {

    DECLARE_bool(save_series);

    struct VariableConfig {
        static VariableConfig &get_instance() {
            static VariableConfig instance;
            return instance;
        }

        bool save_series{true};
        bool quote_vector{true};
        bool abort_on_same_name{true};
        bool var_log_dumped{false};
        bool var_dump{false};
        bool var_dump_interval{false};
        std::string var_dump_file;
        std::string var_dump_include;
        std::string var_dump_exclude;
        std::string var_dump_tabs;
    };

    enum DisplayFilter {
        DISPLAY_ON_HTML = 1,
        DISPLAY_ON_PLAIN_TEXT = 2,
        DISPLAY_ON_ALL = 3,
    };

    class Dumper {
    public:
        virtual ~Dumper() {}

        virtual bool dump(const std::string &name,
                          const std::string_view &description) = 0;

        virtual bool dump_comment(const std::string &, const std::string & /*type*/) {
            return true;
        }
    };

    struct DumpOptions {
        // Constructed with default options.
        DumpOptions();

        // If this is true, string-type values will be quoted.
        bool quote_string;

        // The ? in wildcards. Wildcards in URL need to use another character
        // because ? is reserved.
        char question_mark;

        // Dump variables with matched display_filter
        DisplayFilter display_filter;

        // Name matched by these wildcards (or exact names) are kept.
        std::string white_wildcards;

        // Name matched by these wildcards (or exact names) are skipped.
        std::string black_wildcards;
    };

    struct SeriesOptions {
        SeriesOptions() : fixed_length(true), test_only(false) {}

        bool fixed_length; // useless now
        bool test_only;
    };

    class Variable {
    public:
        Variable() {}

        virtual ~Variable();

        // Implement this method to print the variable into ostream.
        virtual void describe(std::ostream &, bool quote_string) const = 0;

        // string form of describe().
        std::string get_description() const;

#ifdef BAIDU_INTERNAL
        // Get value.
        // If subclass does not override this method, the value is the description
        // and the type is std::string.
        virtual void get_value(boost::any* value) const;
#endif

        // Describe saved series as a json-string into the stream.
        // The output will be ploted by flot.js
        // Returns 0 on success, 1 otherwise(this variable does not save series).
        virtual int describe_series(std::ostream &, const SeriesOptions &) const { return 1; }

        // Expose this variable globally so that it's counted in following
        // functions:
        //   list_exposed
        //   count_exposed
        //   describe_exposed
        //   find_exposed
        // Return 0 on success, -1 otherwise.
        int expose(const std::string_view &name,
                   DisplayFilter display_filter = DISPLAY_ON_ALL) {
            return expose_impl(std::string_view(), name, display_filter);
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
        //       bvar::Adder<int> _error;
        //   };
        //   }  // foo
        //   }  // bar
        // Returns 0 on success, -1 otherwise.
        int expose_as(const std::string_view &prefix,
                      const std::string_view &name,
                      DisplayFilter display_filter = DISPLAY_ON_ALL) {
            return expose_impl(prefix, name, display_filter);
        }

        // Hide this variable so that it's not counted in *_exposed functions.
        // Returns false if this variable is already hidden.
        // CAUTION!! Subclasses must call hide() manually to avoid displaying
        // a variable that is just destructing.
        bool hide();

        // Check if this variable is is_hidden.
        bool is_hidden() const;

        // Get exposed name. If this variable is not exposed, the name is empty.
        const std::string &name() const { return _name; }

        // ====================================================================

        // Put names of all exposed variables into `names'.
        // If you want to print all variables, you have to go through `names'
        // and call `describe_exposed' on each name. This prevents an iteration
        // from taking the lock too long.
        static void list_exposed(std::vector<std::string> *names,
                                 DisplayFilter = DISPLAY_ON_ALL);

        // Get number of exposed variables.
        static size_t count_exposed();

        // Find an exposed variable by `name' and put its description into `os'.
        // Returns 0 on found, -1 otherwise.
        static int describe_exposed(const std::string &name,
                                    std::ostream &os,
                                    bool quote_string = false,
                                    DisplayFilter = DISPLAY_ON_ALL);

        // String form. Returns empty string when not found.
        static std::string describe_exposed(const std::string &name,
                                            bool quote_string = false,
                                            DisplayFilter = DISPLAY_ON_ALL);

        // Describe saved series of variable `name' as a json-string into `os'.
        // The output will be ploted by flot.js
        // Returns 0 on success, 1 when the variable does not save series, -1
        // otherwise (no variable named so).
        static int describe_series_exposed(const std::string &name,
                                           std::ostream &,
                                           const SeriesOptions &);

#ifdef BAIDU_INTERNAL
        // Find an exposed variable by `name' and put its value into `value'.
        // Returns 0 on found, -1 otherwise.
        static int get_exposed(const std::string& name, boost::any* value);
#endif

        // Find all exposed variables matching `white_wildcards' but
        // `black_wildcards' and send them to `dumper'.
        // Use default options when `options' is NULL.
        // Return number of dumped variables, -1 on error.
        static int dump_exposed(Dumper *dumper, const DumpOptions *options);

    protected:
        virtual int expose_impl(const std::string_view &prefix,
                                const std::string_view &name,
                                DisplayFilter display_filter);

    private:
        std::string _name;

        // bvar uses TLS, thus copying/assignment need to copy TLS stuff as well,
        // which is heavy. We disable copying/assignment now.
        TURBO_NON_COPYABLE(Variable);
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

}  // namespace turbo

// Make variables printable.
namespace std {

    inline ostream &operator<<(ostream &os, const ::turbo::Variable &var) {
        var.describe(os, false);
        return os;
    }

}  // namespace std

#endif  // BVAR_VARIABLE_H
