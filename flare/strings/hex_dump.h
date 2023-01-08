
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_STRINGS_HEX_DUMP_H_
#define FLARE_STRINGS_HEX_DUMP_H_

#include <cstdint>
#include <string>
#include <vector>


namespace flare {

// Uppercase Hex_dump Methods

/*!
 * Dump a (binary) string as a sequence of uppercase hexadecimal pairs.
 *
 * \param data  binary data to output in hex
 * \param size  length of binary data
 * \return      string of hexadecimal pairs
 */
std::string hex_dump(const void *const data, size_t size);

/*!
 * Dump a (binary) string as a sequence of uppercase hexadecimal pairs.
 *
 * \param str  binary data to output in hex
 * \return     string of hexadecimal pairs
 */
std::string hex_dump(const std::string &str);

/*!
 * Dump a (binary) item as a sequence of uppercase hexadecimal pairs.
 *
 * \param t  binary data to output in hex
 * \return   string of hexadecimal pairs
 */
template<typename Type>
std::string hex_dump_type(const Type &t) {
    return hex_dump(&t, sizeof(t));
}

/*!
 * Dump a char vector as a sequence of uppercase hexadecimal pairs.
 *
 * \param data  binary data to output in hex
 * \return      string of hexadecimal pairs
 */
std::string hex_dump(const std::vector<char> &data);

/*!
 * Dump a uint8_t vector as a sequence of uppercase hexadecimal pairs.
 *
 * \param data  binary data to output in hex
 * \return      string of hexadecimal pairs
 */
std::string hex_dump(const std::vector<uint8_t> &data);

/*!
 * Dump a (binary) string into a C source code snippet. The snippet defines an
 * array of const uint8_t* holding the data of the string.
 *
 * \param str       string to output as C source array
 * \param var_name  name of the array variable in the outputted code snippet
 * \return          string holding C source snippet
 */
std::string hex_dump_sourcecode(
        const std::string &str, const std::string &var_name = "name");

/******************************************************************************/
// Lowercase hex_dump Methods

/*!
 * Dump a (binary) string as a sequence of lowercase hexadecimal pairs.
 *
 * \param data  binary data to output in hex
 * \param size  length of binary data
 * \return      string of hexadecimal pairs
 */
std::string hex_dump_lc(const void *const data, size_t size);

/*!
 * Dump a (binary) string as a sequence of lowercase hexadecimal pairs.
 *
 * \param str  binary data to output in hex
 * \return     string of hexadecimal pairs
 */
std::string hex_dump_lc(const std::string &str);

/*!
 * Dump a (binary) item as a sequence of lowercase hexadecimal pairs.
 *
 * \param t  binary data to output in hex
 * \return   string of hexadecimal pairs
 */
template<typename Type>
std::string hex_dump_lc_type(const Type &t) {
    return hex_dump_lc(&t, sizeof(t));
}

/*!
 * Dump a char vector as a sequence of lowercase hexadecimal pairs.
 *
 * \param data  binary data to output in hex
 * \return      string of hexadecimal pairs
 */
std::string hex_dump_lc(const std::vector<char> &data);

/*!
 * Dump a uint8_t vector as a sequence of lowercase hexadecimal pairs.
 *
 * \param data  binary data to output in hex
 * \return      string of hexadecimal pairs
 */
std::string hex_dump_lc(const std::vector<uint8_t> &data);

/******************************************************************************/
// Parser for Hex Digit Sequence

/*!
 * Read a string as a sequence of hexadecimal pairs. Converts each pair of
 * hexadecimal digits into a byte of the output string. Throws
 * std::runtime_error() if an unknown letter is encountered.
 *
 * \param str  string to parse as hex digits
 * \return     string of read bytes
 */
std::string parse_hex_dump(const std::string &str);

}  // namespace flare
#endif  // FLARE_STRINGS_HEX_DUMP_H_
