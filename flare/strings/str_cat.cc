
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///

#include "flare/strings/str_cat.h"

#include <assert.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include "flare/base/uninitialized.h"
#include "flare/strings/ascii.h"
#include "flare/strings/numbers.h"

namespace flare {


alpha_num::alpha_num(hex h) {
    static_assert(numbers_internal::kFastToBufferSize >= 32,
                  "This function only works when output buffer >= 32 bytes long");
    char *const end = &digits_[numbers_internal::kFastToBufferSize];
    auto real_width =
            flare::numbers_internal::fast_hex_to_buffer_zero_pad16(h.value, end - 16);
    if (real_width >= h.width) {
        piece_ = std::string_view(end - real_width, real_width);
    } else {
        // Pad first 16 chars because fast_hex_to_buffer_zero_pad16 pads only to 16 and
        // max pad width can be up to 20.
        std::memset(end - 32, h.fill, 16);
        // Patch up everything else up to the real_width.
        std::memset(end - real_width - 16, h.fill, 16);
        piece_ = std::string_view(end - h.width, h.width);
    }
}

alpha_num::alpha_num(dec d) {
    assert(d.width <= numbers_internal::kFastToBufferSize);
    char *const end = &digits_[numbers_internal::kFastToBufferSize];
    char *const minfill = end - d.width;
    char *writer = end;
    uint64_t value = d.value;
    bool neg = d.neg;
    while (value > 9) {
        *--writer = '0' + (value % 10);
        value /= 10;
    }
    *--writer = '0' + value;
    if (neg) *--writer = '-';

    ptrdiff_t fillers = writer - minfill;
    if (fillers > 0) {
        // Tricky: if the fill character is ' ', then it's <fill><+/-><digits>
        // But...: if the fill character is '0', then it's <+/-><fill><digits>
        bool add_sign_again = false;
        if (neg && d.fill == '0') {  // If filling with '0',
            ++writer;                    // ignore the sign we just added
            add_sign_again = true;       // and re-add the sign later.
        }
        writer -= fillers;
        std::fill_n(writer, fillers, d.fill);
        if (add_sign_again) *--writer = '-';
    }

    piece_ = std::string_view(writer, end - writer);
}

// ----------------------------------------------------------------------
// string_cat()
//    This merges the given strings or integers, with no delimiter. This
//    is designed to be the fastest possible way to construct a string out
//    of a mix of raw C strings, string_views, strings, and integer values.
// ----------------------------------------------------------------------

// Append is merely a version of memcpy that returns the address of the byte
// after the area just overwritten.
static char *Append(char *out, const alpha_num &x) {
    // memcpy is allowed to overwrite arbitrary memory, so doing this after the
    // call would force an extra fetch of x.size().
    char *after = out + x.size();
    if (x.size() != 0) {
        memcpy(out, x.data(), x.size());
    }
    return after;
}

std::string string_cat(const alpha_num &a, const alpha_num &b) {
    std::string result;
    flare::base::string_resize_uninitialized(&result,
                                      a.size() + b.size());
    char *const begin = &result[0];
    char *out = begin;
    out = Append(out, a);
    out = Append(out, b);
    assert(out == begin + result.size());
    return result;
}

std::string string_cat(const alpha_num &a, const alpha_num &b, const alpha_num &c) {
    std::string result;
    flare::base::string_resize_uninitialized(
            &result, a.size() + b.size() + c.size());
    char *const begin = &result[0];
    char *out = begin;
    out = Append(out, a);
    out = Append(out, b);
    out = Append(out, c);
    assert(out == begin + result.size());
    return result;
}

std::string string_cat(const alpha_num &a, const alpha_num &b, const alpha_num &c,
                       const alpha_num &d) {
    std::string result;
    flare::base::string_resize_uninitialized(
            &result, a.size() + b.size() + c.size() + d.size());
    char *const begin = &result[0];
    char *out = begin;
    out = Append(out, a);
    out = Append(out, b);
    out = Append(out, c);
    out = Append(out, d);
    assert(out == begin + result.size());
    return result;
}

namespace strings_internal {

// Do not call directly - these are not part of the public API.
std::string CatPieces(std::initializer_list<std::string_view> pieces) {
    std::string result;
    size_t total_size = 0;
    for (const std::string_view &piece : pieces) total_size += piece.size();
    flare::base::string_resize_uninitialized(&result, total_size);

    char *const begin = &result[0];
    char *out = begin;
    for (const std::string_view &piece : pieces) {
        const size_t this_size = piece.size();
        if (this_size != 0) {
            memcpy(out, piece.data(), this_size);
            out += this_size;
        }
    }
    assert(out == begin + result.size());
    return result;
}

// It's possible to call string_append with an std::string_view that is itself a
// fragment of the string we're appending to.  However the results of this are
// random. Therefore, check for this in debug mode.  Use unsigned math so we
// only have to do one comparison. Note, there's an exception case: appending an
// empty string is always allowed.
#define ASSERT_NO_OVERLAP(dest, src) \
  assert(((src).size() == 0) ||      \
         (uintptr_t((src).data() - (dest).data()) > uintptr_t((dest).size())))

void append_pieces(std::string *dest,
                   std::initializer_list<std::string_view> pieces) {
    size_t old_size = dest->size();
    size_t total_size = old_size;
    for (const std::string_view &piece : pieces) {
        ASSERT_NO_OVERLAP(*dest, piece);
        total_size += piece.size();
    }
    flare::base::string_resize_uninitialized(dest, total_size);

    char *const begin = &(*dest)[0];
    char *out = begin + old_size;
    for (const std::string_view &piece : pieces) {
        const size_t this_size = piece.size();
        if (this_size != 0) {
            memcpy(out, piece.data(), this_size);
            out += this_size;
        }
    }
    assert(out == begin + dest->size());
}

}  // namespace strings_internal

void string_append(std::string *dest, const alpha_num &a) {
    ASSERT_NO_OVERLAP(*dest, a);
    dest->append(a.data(), a.size());
}

void string_append(std::string *dest, const alpha_num &a, const alpha_num &b) {
    ASSERT_NO_OVERLAP(*dest, a);
    ASSERT_NO_OVERLAP(*dest, b);
    std::string::size_type old_size = dest->size();
    flare::base::string_resize_uninitialized(
            dest, old_size + a.size() + b.size());
    char *const begin = &(*dest)[0];
    char *out = begin + old_size;
    out = Append(out, a);
    out = Append(out, b);
    assert(out == begin + dest->size());
}

void string_append(std::string *dest, const alpha_num &a, const alpha_num &b,
                   const alpha_num &c) {
    ASSERT_NO_OVERLAP(*dest, a);
    ASSERT_NO_OVERLAP(*dest, b);
    ASSERT_NO_OVERLAP(*dest, c);
    std::string::size_type old_size = dest->size();
    flare::base::string_resize_uninitialized(
            dest, old_size + a.size() + b.size() + c.size());
    char *const begin = &(*dest)[0];
    char *out = begin + old_size;
    out = Append(out, a);
    out = Append(out, b);
    out = Append(out, c);
    assert(out == begin + dest->size());
}

void string_append(std::string *dest, const alpha_num &a, const alpha_num &b,
                   const alpha_num &c, const alpha_num &d) {
    ASSERT_NO_OVERLAP(*dest, a);
    ASSERT_NO_OVERLAP(*dest, b);
    ASSERT_NO_OVERLAP(*dest, c);
    ASSERT_NO_OVERLAP(*dest, d);
    std::string::size_type old_size = dest->size();
    flare::base::string_resize_uninitialized(
            dest, old_size + a.size() + b.size() + c.size() + d.size());
    char *const begin = &(*dest)[0];
    char *out = begin + old_size;
    out = Append(out, a);
    out = Append(out, b);
    out = Append(out, c);
    out = Append(out, d);
    assert(out == begin + dest->size());
}


}  // namespace flare
