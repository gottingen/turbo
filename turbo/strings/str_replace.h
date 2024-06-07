// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// -----------------------------------------------------------------------------
// File: str_replace.h
// -----------------------------------------------------------------------------
//
// This file defines `turbo::str_replace_all()`, a general-purpose string
// replacement function designed for large, arbitrary text substitutions,
// especially on strings which you are receiving from some other system for
// further processing (e.g. processing regular expressions, escaping HTML
// entities, etc.). `str_replace_all` is designed to be efficient even when only
// one substitution is being performed, or when substitution is rare.
//
// If the string being modified is known at compile-time, and the substitutions
// vary, `turbo::substitute()` may be a better choice.
//
// Example:
//
// std::string html_escaped = turbo::str_replace_all(user_input, {
//                                                {"&", "&amp;"},
//                                                {"<", "&lt;"},
//                                                {">", "&gt;"},
//                                                {"\"", "&quot;"},
//                                                {"'", "&#39;"}});

#pragma once

#include <string>
#include <utility>
#include <vector>

#include <turbo/base/attributes.h>
#include <turbo/base/nullability.h>
#include <turbo/strings/string_view.h>

namespace turbo {

    // str_replace_all()
    //
    // Replaces character sequences within a given string with replacements provided
    // within an initializer list of key/value pairs. Candidate replacements are
    // considered in order as they occur within the string, with earlier matches
    // taking precedence, and longer matches taking precedence for candidates
    // starting at the same position in the string. Once a substitution is made, the
    // replaced text is not considered for any further substitutions.
    //
    // Example:
    //
    //   std::string s = turbo::str_replace_all(
    //       "$who bought $count #Noun. Thanks $who!",
    //       {{"$count", turbo::str_cat(5)},
    //        {"$who", "Bob"},
    //        {"#Noun", "Apples"}});
    //   EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
    TURBO_MUST_USE_RESULT std::string str_replace_all(
            std::string_view s,
            std::initializer_list<std::pair<std::string_view, std::string_view>>
            replacements);

    // Overload of `str_replace_all()` to accept a container of key/value replacement
    // pairs (typically either an associative map or a `std::vector` of `std::pair`
    // elements). A vector of pairs is generally more efficient.
    //
    // Examples:
    //
    //   std::map<const std::string_view, const std::string_view> replacements;
    //   replacements["$who"] = "Bob";
    //   replacements["$count"] = "5";
    //   replacements["#Noun"] = "Apples";
    //   std::string s = turbo::str_replace_all(
    //       "$who bought $count #Noun. Thanks $who!",
    //       replacements);
    //   EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
    //
    //   // A std::vector of std::pair elements can be more efficient.
    //   std::vector<std::pair<const std::string_view, std::string>> replacements;
    //   replacements.push_back({"&", "&amp;"});
    //   replacements.push_back({"<", "&lt;"});
    //   replacements.push_back({">", "&gt;"});
    //   std::string s = turbo::str_replace_all("if (ptr < &foo)",
    //                                  replacements);
    //   EXPECT_EQ("if (ptr &lt; &amp;foo)", s);
    template<typename StrToStrMapping>
    std::string str_replace_all(std::string_view s,
                              const StrToStrMapping &replacements);

    // Overload of `str_replace_all()` to replace character sequences within a given
    // output string *in place* with replacements provided within an initializer
    // list of key/value pairs, returning the number of substitutions that occurred.
    //
    // Example:
    //
    //   std::string s = std::string("$who bought $count #Noun. Thanks $who!");
    //   int count;
    //   count = turbo::str_replace_all({{"$count", turbo::str_cat(5)},
    //                               {"$who", "Bob"},
    //                               {"#Noun", "Apples"}}, &s);
    //  EXPECT_EQ(count, 4);
    //  EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
    int str_replace_all(
            std::initializer_list<std::pair<std::string_view, std::string_view>>
            replacements,
            turbo::Nonnull<std::string *> target);

    // Overload of `str_replace_all()` to replace patterns within a given output
    // string *in place* with replacements provided within a container of key/value
    // pairs.
    //
    // Example:
    //
    //   std::string s = std::string("if (ptr < &foo)");
    //   int count = turbo::str_replace_all({{"&", "&amp;"},
    //                                    {"<", "&lt;"},
    //                                    {">", "&gt;"}}, &s);
    //  EXPECT_EQ(count, 2);
    //  EXPECT_EQ("if (ptr &lt; &amp;foo)", s);
    template<typename StrToStrMapping>
    int str_replace_all(const StrToStrMapping &replacements,
                      turbo::Nonnull<std::string *> target);

    // Implementation details only, past this point.
    namespace strings_internal {

        struct ViableSubstitution {
            std::string_view old;
            std::string_view replacement;
            size_t offset;

            ViableSubstitution(std::string_view old_str,
                               std::string_view replacement_str, size_t offset_val)
                    : old(old_str), replacement(replacement_str), offset(offset_val) {}

            // One substitution occurs "before" another (takes priority) if either
            // it has the lowest offset, or it has the same offset but a larger size.
            bool OccursBefore(const ViableSubstitution &y) const {
                if (offset != y.offset) return offset < y.offset;
                return old.size() > y.old.size();
            }
        };

        // Build a vector of ViableSubstitutions based on the given list of
        // replacements. subs can be implemented as a priority_queue. However, it turns
        // out that most callers have small enough a list of substitutions that the
        // overhead of such a queue isn't worth it.
        template<typename StrToStrMapping>
        std::vector<ViableSubstitution> FindSubstitutions(
                std::string_view s, const StrToStrMapping &replacements) {
            std::vector<ViableSubstitution> subs;
            subs.reserve(replacements.size());

            for (const auto &rep: replacements) {
                using std::get;
                std::string_view old(get<0>(rep));

                size_t pos = s.find(old);
                if (pos == s.npos) continue;

                // Ignore attempts to replace "". This condition is almost never true,
                // but above condition is frequently true. That's why we test for this
                // now and not before.
                if (old.empty()) continue;

                subs.emplace_back(old, get<1>(rep), pos);

                // Insertion sort to ensure the last ViableSubstitution comes before
                // all the others.
                size_t index = subs.size();
                while (--index && subs[index - 1].OccursBefore(subs[index])) {
                    std::swap(subs[index], subs[index - 1]);
                }
            }
            return subs;
        }

        int ApplySubstitutions(std::string_view s,
                               turbo::Nonnull<std::vector<ViableSubstitution> *> subs_ptr,
                               turbo::Nonnull<std::string *> result_ptr);

    }  // namespace strings_internal

    template<typename StrToStrMapping>
    std::string str_replace_all(std::string_view s,
                              const StrToStrMapping &replacements) {
        auto subs = strings_internal::FindSubstitutions(s, replacements);
        std::string result;
        result.reserve(s.size());
        strings_internal::ApplySubstitutions(s, &subs, &result);
        return result;
    }

    template<typename StrToStrMapping>
    int str_replace_all(const StrToStrMapping &replacements,
                      turbo::Nonnull<std::string *> target) {
        auto subs = strings_internal::FindSubstitutions(*target, replacements);
        if (subs.empty()) return 0;

        std::string result;
        result.reserve(target->size());
        int substitutions =
                strings_internal::ApplySubstitutions(*target, &subs, &result);
        target->swap(result);
        return substitutions;
    }
}  // namespace turbo
