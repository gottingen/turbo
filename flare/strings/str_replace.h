
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
//
// -----------------------------------------------------------------------------
// File: str_replace.h
// -----------------------------------------------------------------------------
//
// This file defines `flare::string_replace_all()`, a general-purpose string
// replacement function designed for large, arbitrary text substitutions,
// especially on strings which you are receiving from some other system for
// further processing (e.g. processing regular expressions, escaping HTML
// entities, etc.). `string_replace_all` is designed to be efficient even when only
// one substitution is being performed, or when substitution is rare.
//
// If the string being modified is known at compile-time, and the substitutions
// vary, `flare::Substitute()` may be a better choice.
//
// Example:
//
// std::string html_escaped = flare::string_replace_all(user_input, {
//                                                {"&", "&amp;"},
//                                                {"<", "&lt;"},
//                                                {">", "&gt;"},
//                                                {"\"", "&quot;"},
//                                                {"'", "&#39;"}});
#ifndef FLARE_STRINGS_STR_REPLACE_H_
#define FLARE_STRINGS_STR_REPLACE_H_

#include <string>
#include <utility>
#include <vector>

#include "flare/base/profile.h"
#include <string_view>

namespace flare {


// string_replace_all()
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
//   std::string s = flare::string_replace_all(
//       "$who bought $count #Noun. Thanks $who!",
//       {{"$count", flare::string_cat(5)},
//        {"$who", "Bob"},
//        {"#Noun", "Apples"}});
//   EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
FLARE_MUST_USE_RESULT std::string string_replace_all(
        std::string_view s,
        std::initializer_list<std::pair<std::string_view, std::string_view>>
        replacements);

// Overload of `string_replace_all()` to accept a container of key/value replacement
// pairs (typically either an associative map or a `std::vector` of `std::pair`
// elements). A vector of pairs is generally more efficient.
//
// Examples:
//
//   std::map<const std::string_view, const std::string_view> replacements;
//   replacements["$who"] = "Bob";
//   replacements["$count"] = "5";
//   replacements["#Noun"] = "Apples";
//   std::string s = flare::string_replace_all(
//       "$who bought $count #Noun. Thanks $who!",
//       replacements);
//   EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
//
//   // A std::vector of std::pair elements can be more efficient.
//   std::vector<std::pair<const std::string_view, std::string>> replacements;
//   replacements.push_back({"&", "&amp;"});
//   replacements.push_back({"<", "&lt;"});
//   replacements.push_back({">", "&gt;"});
//   std::string s = flare::string_replace_all("if (ptr < &foo)",
//                                  replacements);
//   EXPECT_EQ("if (ptr &lt; &amp;foo)", s);
template<typename StrToStrMapping>
std::string string_replace_all(std::string_view s,
                               const StrToStrMapping &replacements);

// Overload of `string_replace_all()` to replace character sequences within a given
// output string *in place* with replacements provided within an initializer
// list of key/value pairs, returning the number of substitutions that occurred.
//
// Example:
//
//   std::string s = std::string("$who bought $count #Noun. Thanks $who!");
//   int count;
//   count = flare::string_replace_all({{"$count", flare::string_cat(5)},
//                               {"$who", "Bob"},
//                               {"#Noun", "Apples"}}, &s);
//  EXPECT_EQ(count, 4);
//  EXPECT_EQ("Bob bought 5 Apples. Thanks Bob!", s);
int string_replace_all(
        std::initializer_list<std::pair<std::string_view, std::string_view>>
        replacements,
        std::string *target);

// Overload of `string_replace_all()` to replace patterns within a given output
// string *in place* with replacements provided within a container of key/value
// pairs.
//
// Example:
//
//   std::string s = std::string("if (ptr < &foo)");
//   int count = flare::string_replace_all({{"&", "&amp;"},
//                                    {"<", "&lt;"},
//                                    {">", "&gt;"}}, &s);
//  EXPECT_EQ(count, 2);
//  EXPECT_EQ("if (ptr &lt; &amp;foo)", s);
template<typename StrToStrMapping>
int string_replace_all(const StrToStrMapping &replacements, std::string *target);

// Implementation details only, past this point.
namespace strings_internal {

struct viable_substitution {
    std::string_view old;
    std::string_view replacement;
    size_t offset;

    viable_substitution(std::string_view old_str,
                        std::string_view replacement_str, size_t offset_val)
            : old(old_str), replacement(replacement_str), offset(offset_val) {}

    // One substitution occurs "before" another (takes priority) if either
    // it has the lowest offset, or it has the same offset but a larger size.
    bool OccursBefore(const viable_substitution &y) const {
        if (offset != y.offset) return offset < y.offset;
        return old.size() > y.old.size();
    }
};

// Build a vector of ViableSubstitutions based on the given list of
// replacements. subs can be implemented as a priority_queue. However, it turns
// out that most callers have small enough a list of substitutions that the
// overhead of such a queue isn't worth it.
template<typename StrToStrMapping>
std::vector<viable_substitution> find_substitutions(
        std::string_view s, const StrToStrMapping &replacements) {
    std::vector<viable_substitution> subs;
    subs.reserve(replacements.size());

    for (const auto &rep : replacements) {
        using std::get;
        std::string_view old(get<0>(rep));

        size_t pos = s.find(old);
        if (pos == s.npos) continue;

        // Ignore attempts to replace "". This condition is almost never true,
        // but above condition is frequently true. That's why we test for this
        // now and not before.
        if (old.empty()) continue;

        subs.emplace_back(old, get<1>(rep), pos);

        // Insertion sort to ensure the last viable_substitution comes before
        // all the others.
        size_t index = subs.size();
        while (--index && subs[index - 1].OccursBefore(subs[index])) {
            std::swap(subs[index], subs[index - 1]);
        }
    }
    return subs;
}

int apply_substitutions(std::string_view s,
                        std::vector<viable_substitution> *subs_ptr,
                        std::string *result_ptr);

}  // namespace strings_internal

template<typename StrToStrMapping>
std::string string_replace_all(std::string_view s,
                               const StrToStrMapping &replacements) {
    auto subs = strings_internal::find_substitutions(s, replacements);
    std::string result;
    result.reserve(s.size());
    strings_internal::apply_substitutions(s, &subs, &result);
    return result;
}

template<typename StrToStrMapping>
int string_replace_all(const StrToStrMapping &replacements, std::string *target) {
    auto subs = strings_internal::find_substitutions(*target, replacements);
    if (subs.empty()) return 0;

    std::string result;
    result.reserve(target->size());
    int substitutions =
            strings_internal::apply_substitutions(*target, &subs, &result);
    target->swap(result);
    return substitutions;
}


}  // namespace flare

#endif  // FLARE_STRINGS_STR_REPLACE_H_
