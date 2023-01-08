
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_BASE_BASE64_H_
#define FLARE_BASE_BASE64_H_

#include <string>
#include <string_view>

namespace flare::base {
    // Base64 Encoding and Decoding

    /**
     * @brief Encode the given binary string into base64 representation as described in RFC
     * 2045 or RFC 3548. The output string contains only characters [A-Za-z0-9+/]
     * and is roughly 33% longer than the input. The output string can be broken
     * into lines after n characters, where n must be a multiple of 4.
     * @param str input string to encode
     * @param line_break break the output string every n characters
     * @return base64 encoded string
     */
    bool base64_encode(std::string_view str, std::string *out, size_t line_break = 0);


    /**
     * @brief Decode a string in base64 representation as described in RFC 2045 or RFC 3548
     * and return the original data. If a non-whitespace invalid base64 character is
     * encountered _and_ the parameter "strict" is true, then this function will
     * throw a std::runtime_error. If "strict" is false, the character is silently
     * ignored.
     * @param str input string to encode
     * @param strict throw exception on invalid character
     * @return decoded binary data
     */
    bool base64_decode(std::string_view str, std::string *out, bool strict = true);

}  //  namespace namespace flare::base

#endif  // FLARE_BASE_BASE64_H_
