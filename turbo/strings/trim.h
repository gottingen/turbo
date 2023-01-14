
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/
#ifndef TURBO_STRINGS_TRIM_H_
#define TURBO_STRINGS_TRIM_H_

#include <string_view>
#include <string>
#include <algorithm>
#include "turbo/base/profile.h"
#include "turbo/strings/ascii.h"
#include "turbo/strings/ends_with.h"
#include "turbo/strings/starts_with.h"

namespace turbo {
    ///
    /// Trims the given string in-place only on the right. Removes all characters in
    /// the given drop array, which defaults to " \r\n\t".
    ///
    /// \param str   string to process
    /// \return      reference to the modified string
    ///
    std::string &trim_inplace_right(std::string *str);

    ///
    /// Trims the given string in-place only on the right. Removes all characters in
    /// the given drop array.
    ///
    /// \param str   string to process
    /// \param drop  remove these characters
    /// \return      reference to the modified string
    ///
    std::string &trim_inplace_right(std::string *str, std::string_view drop);

    /*!
     * Trims the given string only on the right. Removes all characters in the
     * given drop array. Returns a copy of the string.
     *
     * \param str   string to process
     * \param drop  remove these characters
     * \return      new trimmed string
     */
    std::string_view trim_right(std::string_view str, std::string_view drop);

    /**
     * @brief todo
     * @param str todo
     * @return todo
     */
    TURBO_MUST_USE_RESULT TURBO_FORCE_INLINE std::string_view trim_right(std::string_view str) {
        auto it = std::find_if_not(str.rbegin(), str.rend(), turbo::ascii::is_space);
        return str.substr(0, str.rend() - it);
    }

    /******************************************************************************/

    /*!
     * Trims the given string in-place only on the left. Removes all characters in
     * the given drop array.
     *
     * \param str   string to process
     * \return      reference to the modified string
     */
    std::string &trim_inplace_left(std::string *str);

    /*!
     * Trims the given string in-place only on the left. Removes all characters in
     * the given drop array.
     *
     * \param str   string to process
     * \param drop  remove these characters
     * \return      reference to the modified string
     */
    std::string &trim_inplace_left(std::string *str, std::string_view drop);

    /*!
     * Trims the given string only on the left. Removes all characters in the given
     * drop array. Returns a copy of the string.
     *
     * \param str   string to process
     * \param drop  remove these characters
     * \return      new trimmed string
     */
    std::string_view trim_left(std::string_view str, std::string_view drop);

    TURBO_MUST_USE_RESULT TURBO_FORCE_INLINE std::string_view trim_left(std::string_view str) {
        auto it = std::find_if_not(str.begin(), str.end(), turbo::ascii::is_space);
        return str.substr(it - str.begin());
    }

    /*!
     * Trims the given string in-place on the left and right. Removes all
     * characters in the given drop array, which defaults to " \r\n\t".
     *
     * \param str   string to process
     * \return      reference to the modified string
     */
    std::string &trim_inplace_all(std::string *str);

    /*!
     * Trims the given string in-place on the left and right. Removes all
     * characters in the given drop array, which defaults to " \r\n\t".
     *
     * \param str   string to process
     * \param drop  remove these characters
     * \return      reference to the modified string
     */
    std::string &trim_inplace_all(std::string *str, std::string_view drop);


    /*!
     * Trims the given string in-place on the left and right. Removes all
     * characters in the given drop array, which defaults to " \r\n\t".
     *
     * \param str   string to process
     * \param drop  remove these characters
     * \return      reference to the modified string
     */
    std::string_view trim_all(std::string_view str, std::string_view drop);

    TURBO_MUST_USE_RESULT TURBO_FORCE_INLINE std::string_view trim_all(std::string_view str) {
        return trim_right(trim_left(str));
    }

    void trim_inplace_complete(std::string *);

    std::string trim_complete(std::string_view str);


}  // namespace turbo

#endif  // TURBO_STRINGS_TRIM_H_
