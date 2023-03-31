//
// Copyright 2020 The Turbo Authors.
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
// -----------------------------------------------------------------------------
// File: string_piece.h
// -----------------------------------------------------------------------------
//
// This file contains the definition of the `turbo::string_piece` class. A
// `string_piece` points to a contiguous span of characters, often part or all of
// another `std::string`, double-quoted string literal, character array, or even
// another `string_piece`.
//
// This `turbo::string_piece` abstraction is designed to be a drop-in
// replacement for the C++17 `turbo::string_piece` abstraction.
#ifndef TURBO_STRINGS_STRING_PIECE_H_
#define TURBO_STRINGS_STRING_PIECE_H_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <string>

#include "turbo/base/internal/throw_delegate.h"
#include "turbo/strings/string_view.h"
#include "turbo/platform/port.h"

#if TURBO_HAVE_BUILTIN(__builtin_memcmp) ||        \
    (defined(__GNUC__) && !defined(__clang__)) || \
    (defined(_MSC_VER) && _MSC_VER >= 1928)
#define TURBO_INTERNAL_STRING_VIEW_MEMCMP __builtin_memcmp
#else  // TURBO_HAVE_BUILTIN(__builtin_memcmp)
#define TURBO_INTERNAL_STRING_VIEW_MEMCMP memcmp
#endif  // TURBO_HAVE_BUILTIN(__builtin_memcmp)

namespace turbo {
TURBO_NAMESPACE_BEGIN

// turbo::string_piece
//
// A `string_piece` provides a lightweight view into the string data provided by
// a `std::string`, double-quoted string literal, character array, or even
// another `string_piece`. A `string_piece` does *not* own the string to which it
// points, and that data cannot be modified through the view.
//
// You can use `string_piece` as a function or method parameter anywhere a
// parameter can receive a double-quoted string literal, `const char*`,
// `std::string`, or another `turbo::string_piece` argument with no need to copy
// the string data. Systematic use of `string_piece` within function arguments
// reduces data copies and `strlen()` calls.
//
// Because of its small size, prefer passing `string_piece` by value:
//
//   void MyFunction(turbo::string_piece arg);
//
// If circumstances require, you may also pass one by const reference:
//
//   void MyFunction(const turbo::string_piece& arg);  // not preferred
//
// Passing by value generates slightly smaller code for many architectures.
//
// In either case, the source data of the `string_piece` must outlive the
// `string_piece` itself.
//
// A `string_piece` is also suitable for local variables if you know that the
// lifetime of the underlying object is longer than the lifetime of your
// `string_piece` variable. However, beware of binding a `string_piece` to a
// temporary value:
//
//   // BAD use of string_piece: lifetime problem
//   turbo::string_piece sv = obj.ReturnAString();
//
//   // GOOD use of string_piece: str outlives sv
//   std::string str = obj.ReturnAString();
//   turbo::string_piece sv = str;
//
// Due to lifetime issues, a `string_piece` is sometimes a poor choice for a
// return value and usually a poor choice for a data member. If you do use a
// `string_piece` this way, it is your responsibility to ensure that the object
// pointed to by the `string_piece` outlives the `string_piece`.
//
// A `string_piece` may represent a whole string or just part of a string. For
// example, when splitting a string, `std::vector<turbo::string_piece>` is a
// natural data type for the output.
//
// For another example, a Cord is a non-contiguous, potentially very
// long string-like object.  The Cord class has an interface that iteratively
// provides string_piece objects that point to the successive pieces of a Cord
// object.
//
// When constructed from a source which is NUL-terminated, the `string_piece`
// itself will not include the NUL-terminator unless a specific size (including
// the NUL) is passed to the constructor. As a result, common idioms that work
// on NUL-terminated strings do not work on `string_piece` objects. If you write
// code that scans a `string_piece`, you must check its length rather than test
// for nul, for example. Note, however, that nuls may still be embedded within
// a `string_piece` explicitly.
//
// You may create a null `string_piece` in two ways:
//
//   turbo::string_piece sv;
//   turbo::string_piece sv(nullptr, 0);
//
// For the above, `sv.data() == nullptr`, `sv.length() == 0`, and
// `sv.empty() == true`. Also, if you create a `string_piece` with a non-null
// pointer then `sv.data() != nullptr`. Thus, you can use `string_piece()` to
// signal an undefined value that is different from other `string_piece` values
// in a similar fashion to how `const char* p1 = nullptr;` is different from
// `const char* p2 = "";`. However, in practice, it is not recommended to rely
// on this behavior.
//
// Be careful not to confuse a null `string_piece` with an empty one. A null
// `string_piece` is an empty `string_piece`, but some empty `string_piece`s are
// not null. Prefer checking for emptiness over checking for null.
//
// There are many ways to create an empty string_piece:
//
//   const char* nullcp = nullptr;
//   // string_piece.size() will return 0 in all cases.
//   turbo::string_piece();
//   turbo::string_piece(nullcp, 0);
//   turbo::string_piece("");
//   turbo::string_piece("", 0);
//   turbo::string_piece("abcdef", 0);
//   turbo::string_piece("abcdef" + 6, 0);
//
// All empty `string_piece` objects whether null or not, are equal:
//
//   turbo::string_piece() == turbo::string_piece("", 0)
//   turbo::string_piece(nullptr, 0) == turbo::string_piece("abcdef"+6, 0)
class string_piece {
 public:
  using traits_type = std::char_traits<char>;
  using value_type = char;
  using pointer = char*;
  using const_pointer = const char*;
  using reference = char&;
  using const_reference = const char&;
  using const_iterator = const char*;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;

  static constexpr size_type npos = static_cast<size_type>(-1);

  static constexpr int32_t kUnknownCategory = INT8_MIN;

  // Null `string_piece` constructor
  constexpr string_piece() noexcept : ptr_(nullptr), length_(0), category_(kUnknownCategory) {}

  // Implicit constructors

  template <typename Allocator>
  string_piece(  // NOLINT(runtime/explicit)
      const std::basic_string<char, std::char_traits<char>, Allocator>& str
          TURBO_ATTRIBUTE_LIFETIME_BOUND) noexcept
      // This is implemented in terms of `string_piece(p, n)` so `str.size()`
      // doesn't need to be reevaluated after `ptr_` is set.
      // The length check is also skipped since it is unnecessary and causes
      // code bloat.
      : string_piece(str.data(), str.size(), SkipCheckLengthTag{}) {}

  string_piece(  // NOLINT(runtime/explicit)
      const turbo::basic_string_view<char, std::char_traits<char>>& str
          TURBO_ATTRIBUTE_LIFETIME_BOUND) noexcept
      // This is implemented in terms of `string_piece(p, n)` so `str.size()`
      // doesn't need to be reevaluated after `ptr_` is set.
      // The length check is also skipped since it is unnecessary and causes
      // code bloat.
      : string_piece(str.data(), str.size(), SkipCheckLengthTag{}) {}

  // Implicit constructor of a `string_piece` from NUL-terminated `str`. When
  // accepting possibly null strings, use `turbo::NullSafeStringView(str)`
  // instead (see below).
  // The length check is skipped since it is unnecessary and causes code bloat.
  constexpr string_piece(const char* str)  // NOLINT(runtime/explicit)
      : ptr_(str), length_(str ? StrlenInternal(str) : 0), category_(kUnknownCategory){}

  // Implicit constructor of a `string_piece` from a `const char*` and length.
  constexpr string_piece(const char* data, size_type len)
      : ptr_(data), length_(CheckLengthInternal(len)), category_(kUnknownCategory) {}

  // for some case use it as string pieces
  constexpr string_piece(const char* data, size_type len, int32_t category)
      : ptr_(data), length_(CheckLengthInternal(len)), category_(category) {}

  // NOTE: Harmlessly omitted to work around gdb bug.
  //   constexpr string_piece(const string_piece&) noexcept = default;
  //   string_piece& operator=(const string_piece&) noexcept = default;

  // Iterators

  // string_piece::begin()
  //
  // Returns an iterator pointing to the first character at the beginning of the
  // `string_piece`, or `end()` if the `string_piece` is empty.
  constexpr const_iterator begin() const noexcept { return ptr_; }

  // string_piece::end()
  //
  // Returns an iterator pointing just beyond the last character at the end of
  // the `string_piece`. This iterator acts as a placeholder; attempting to
  // access it results in undefined behavior.
  constexpr const_iterator end() const noexcept { return ptr_ + length_; }

  // string_piece::cbegin()
  //
  // Returns a const iterator pointing to the first character at the beginning
  // of the `string_piece`, or `end()` if the `string_piece` is empty.
  constexpr const_iterator cbegin() const noexcept { return begin(); }

  // string_piece::cend()
  //
  // Returns a const iterator pointing just beyond the last character at the end
  // of the `string_piece`. This pointer acts as a placeholder; attempting to
  // access its element results in undefined behavior.
  constexpr const_iterator cend() const noexcept { return end(); }

  // string_piece::rbegin()
  //
  // Returns a reverse iterator pointing to the last character at the end of the
  // `string_piece`, or `rend()` if the `string_piece` is empty.
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  // string_piece::rend()
  //
  // Returns a reverse iterator pointing just before the first character at the
  // beginning of the `string_piece`. This pointer acts as a placeholder;
  // attempting to access its element results in undefined behavior.
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  // string_piece::crbegin()
  //
  // Returns a const reverse iterator pointing to the last character at the end
  // of the `string_piece`, or `crend()` if the `string_piece` is empty.
  const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  // string_piece::crend()
  //
  // Returns a const reverse iterator pointing just before the first character
  // at the beginning of the `string_piece`. This pointer acts as a placeholder;
  // attempting to access its element results in undefined behavior.
  const_reverse_iterator crend() const noexcept { return rend(); }

  // Capacity Utilities

  // string_piece::size()
  //
  // Returns the number of characters in the `string_piece`.
  constexpr size_type size() const noexcept { return length_; }

  // string_piece::length()
  //
  // Returns the number of characters in the `string_piece`. Alias for `size()`.
  constexpr size_type length() const noexcept { return size(); }

  // string_piece::max_size()
  //
  // Returns the maximum number of characters the `string_piece` can hold.
  constexpr size_type max_size() const noexcept { return kMaxSize; }

  // string_piece::empty()
  //
  // Checks if the `string_piece` is empty (refers to no characters).
  constexpr bool empty() const noexcept { return length_ == 0; }

  // string_piece::operator[]
  //
  // Returns the ith element of the `string_piece` using the array operator.
  // Note that this operator does not perform any bounds checking.
  constexpr const_reference operator[](size_type i) const {
    return TURBO_HARDENING_ASSERT(i < size()), ptr_[i];
  }

  // string_piece::at()
  //
  // Returns the ith element of the `string_piece`. Bounds checking is performed,
  // and an exception of type `std::out_of_range` will be thrown on invalid
  // access.
  constexpr const_reference at(size_type i) const {
    return TURBO_LIKELY(i < size())
               ? ptr_[i]
               : ((void)base_internal::ThrowStdOutOfRange(
                      "turbo::string_piece::at"),
                  ptr_[i]);
  }

  // string_piece::front()
  //
  // Returns the first element of a `string_piece`.
  constexpr const_reference front() const {
    return TURBO_HARDENING_ASSERT(!empty()), ptr_[0];
  }

  // string_piece::back()
  //
  // Returns the last element of a `string_piece`.
  constexpr const_reference back() const {
    return TURBO_HARDENING_ASSERT(!empty()), ptr_[size() - 1];
  }

  // string_piece::data()
  //
  // Returns a pointer to the underlying character array (which is of course
  // stored elsewhere). Note that `string_piece::data()` may contain embedded nul
  // characters, but the returned buffer may or may not be NUL-terminated;
  // therefore, do not pass `data()` to a routine that expects a NUL-terminated
  // string.
  constexpr const_pointer data() const noexcept { return ptr_; }

  // Modifiers

  // string_piece::remove_prefix()
  //
  // Removes the first `n` characters from the `string_piece`. Note that the
  // underlying string is not changed, only the view.
  constexpr void remove_prefix(size_type n) {
    TURBO_HARDENING_ASSERT(n <= length_);
    ptr_ += n;
    length_ -= n;
  }

  // string_piece::remove_suffix()
  //
  // Removes the last `n` characters from the `string_piece`. Note that the
  // underlying string is not changed, only the view.
  constexpr void remove_suffix(size_type n) {
    TURBO_HARDENING_ASSERT(n <= length_);
    length_ -= n;
  }

  // string_piece::swap()
  //
  // Swaps this `string_piece` with another `string_piece`.
  constexpr void swap(string_piece& s) noexcept {
    auto t = *this;
    *this = s;
    s = t;
  }

  // Explicit conversion operators

  // Converts to `std::basic_string`.
  template <typename A>
  explicit operator std::basic_string<char, traits_type, A>() const {
    if (!data()) return {};
    return std::basic_string<char, traits_type, A>(data(), size());
  }

  // string_piece::copy()
  //
  // Copies the contents of the `string_piece` at offset `pos` and length `n`
  // into `buf`.
  size_type copy(char* buf, size_type n, size_type pos = 0) const {
    if (TURBO_UNLIKELY(pos > length_)) {
      base_internal::ThrowStdOutOfRange("turbo::string_piece::copy");
    }
    size_type rlen = (std::min)(length_ - pos, n);
    if (rlen > 0) {
      const char* start = ptr_ + pos;
      traits_type::copy(buf, start, rlen);
    }
    return rlen;
  }

  // string_piece::substr()
  //
  // Returns a "substring" of the `string_piece` (at offset `pos` and length
  // `n`) as another string_piece. This function throws `std::out_of_bounds` if
  // `pos > size`.
  // Use turbo::ClippedSubstr if you need a truncating substr operation.
  constexpr string_piece substr(size_type pos = 0, size_type n = npos) const {
    return TURBO_UNLIKELY(pos > length_)
               ? (base_internal::ThrowStdOutOfRange(
                      "turbo::string_piece::substr"),
                  string_piece())
               : string_piece(ptr_ + pos, Min(n, length_ - pos));
  }

  // string_piece::compare()
  //
  // Performs a lexicographical comparison between this `string_piece` and
  // another `string_piece` `x`, returning a negative value if `*this` is less
  // than `x`, 0 if `*this` is equal to `x`, and a positive value if `*this`
  // is greater than `x`.
  constexpr int compare(string_piece x) const noexcept {
    return CompareImpl(length_, x.length_,
                       Min(length_, x.length_) == 0
                           ? 0
                           : TURBO_INTERNAL_STRING_VIEW_MEMCMP(
                                 ptr_, x.ptr_, Min(length_, x.length_)));
  }

  // Overload of `string_piece::compare()` for comparing a substring of the
  // 'string_piece` and another `turbo::string_piece`.
  constexpr int compare(size_type pos1, size_type count1, string_piece v) const {
    return substr(pos1, count1).compare(v);
  }

  // Overload of `string_piece::compare()` for comparing a substring of the
  // `string_piece` and a substring of another `turbo::string_piece`.
  constexpr int compare(size_type pos1, size_type count1, string_piece v,
                        size_type pos2, size_type count2) const {
    return substr(pos1, count1).compare(v.substr(pos2, count2));
  }

  // Overload of `string_piece::compare()` for comparing a `string_piece` and a
  // a different C-style string `s`.
  constexpr int compare(const char* s) const { return compare(string_piece(s)); }

  // Overload of `string_piece::compare()` for comparing a substring of the
  // `string_piece` and a different string C-style string `s`.
  constexpr int compare(size_type pos1, size_type count1, const char* s) const {
    return substr(pos1, count1).compare(string_piece(s));
  }

  // Overload of `string_piece::compare()` for comparing a substring of the
  // `string_piece` and a substring of a different C-style string `s`.
  constexpr int compare(size_type pos1, size_type count1, const char* s,
                        size_type count2) const {
    return substr(pos1, count1).compare(string_piece(s, count2));
  }

  // Find Utilities

  // string_piece::find()
  //
  // Finds the first occurrence of the substring `s` within the `string_piece`,
  // returning the position of the first character's match, or `npos` if no
  // match was found.
  size_type find(string_piece s, size_type pos = 0) const noexcept;

  // Overload of `string_piece::find()` for finding the given character `c`
  // within the `string_piece`.
  size_type find(char c, size_type pos = 0) const noexcept;

  // Overload of `string_piece::find()` for finding a substring of a different
  // C-style string `s` within the `string_piece`.
  size_type find(const char* s, size_type pos, size_type count) const {
    return find(string_piece(s, count), pos);
  }

  // Overload of `string_piece::find()` for finding a different C-style string
  // `s` within the `string_piece`.
  size_type find(const char* s, size_type pos = 0) const {
    return find(string_piece(s), pos);
  }

  // string_piece::rfind()
  //
  // Finds the last occurrence of a substring `s` within the `string_piece`,
  // returning the position of the first character's match, or `npos` if no
  // match was found.
  size_type rfind(string_piece s, size_type pos = npos) const noexcept;

  // Overload of `string_piece::rfind()` for finding the last given character `c`
  // within the `string_piece`.
  size_type rfind(char c, size_type pos = npos) const noexcept;

  // Overload of `string_piece::rfind()` for finding a substring of a different
  // C-style string `s` within the `string_piece`.
  size_type rfind(const char* s, size_type pos, size_type count) const {
    return rfind(string_piece(s, count), pos);
  }

  // Overload of `string_piece::rfind()` for finding a different C-style string
  // `s` within the `string_piece`.
  size_type rfind(const char* s, size_type pos = npos) const {
    return rfind(string_piece(s), pos);
  }

  // string_piece::find_first_of()
  //
  // Finds the first occurrence of any of the characters in `s` within the
  // `string_piece`, returning the start position of the match, or `npos` if no
  // match was found.
  size_type find_first_of(string_piece s, size_type pos = 0) const noexcept;

  // Overload of `string_piece::find_first_of()` for finding a character `c`
  // within the `string_piece`.
  size_type find_first_of(char c, size_type pos = 0) const noexcept {
    return find(c, pos);
  }

  // Overload of `string_piece::find_first_of()` for finding a substring of a
  // different C-style string `s` within the `string_piece`.
  size_type find_first_of(const char* s, size_type pos,
                                    size_type count) const {
    return find_first_of(string_piece(s, count), pos);
  }

  // Overload of `string_piece::find_first_of()` for finding a different C-style
  // string `s` within the `string_piece`.
  size_type find_first_of(const char* s, size_type pos = 0) const {
    return find_first_of(string_piece(s), pos);
  }

  // string_piece::find_last_of()
  //
  // Finds the last occurrence of any of the characters in `s` within the
  // `string_piece`, returning the start position of the match, or `npos` if no
  // match was found.
  size_type find_last_of(string_piece s, size_type pos = npos) const noexcept;

  // Overload of `string_piece::find_last_of()` for finding a character `c`
  // within the `string_piece`.
  size_type find_last_of(char c, size_type pos = npos) const noexcept {
    return rfind(c, pos);
  }

  // Overload of `string_piece::find_last_of()` for finding a substring of a
  // different C-style string `s` within the `string_piece`.
  size_type find_last_of(const char* s, size_type pos, size_type count) const {
    return find_last_of(string_piece(s, count), pos);
  }

  // Overload of `string_piece::find_last_of()` for finding a different C-style
  // string `s` within the `string_piece`.
  size_type find_last_of(const char* s, size_type pos = npos) const {
    return find_last_of(string_piece(s), pos);
  }

  // string_piece::find_first_not_of()
  //
  // Finds the first occurrence of any of the characters not in `s` within the
  // `string_piece`, returning the start position of the first non-match, or
  // `npos` if no non-match was found.
  size_type find_first_not_of(string_piece s, size_type pos = 0) const noexcept;

  // Overload of `string_piece::find_first_not_of()` for finding a character
  // that is not `c` within the `string_piece`.
  size_type find_first_not_of(char c, size_type pos = 0) const noexcept;

  // Overload of `string_piece::find_first_not_of()` for finding a substring of a
  // different C-style string `s` within the `string_piece`.
  size_type find_first_not_of(const char* s, size_type pos,
                              size_type count) const {
    return find_first_not_of(string_piece(s, count), pos);
  }

  // Overload of `string_piece::find_first_not_of()` for finding a different
  // C-style string `s` within the `string_piece`.
  size_type find_first_not_of(const char* s, size_type pos = 0) const {
    return find_first_not_of(string_piece(s), pos);
  }

  // string_piece::find_last_not_of()
  //
  // Finds the last occurrence of any of the characters not in `s` within the
  // `string_piece`, returning the start position of the last non-match, or
  // `npos` if no non-match was found.
  size_type find_last_not_of(string_piece s,
                             size_type pos = npos) const noexcept;

  // Overload of `string_piece::find_last_not_of()` for finding a character
  // that is not `c` within the `string_piece`.
  size_type find_last_not_of(char c, size_type pos = npos) const noexcept;

  // Overload of `string_piece::find_last_not_of()` for finding a substring of a
  // different C-style string `s` within the `string_piece`.
  size_type find_last_not_of(const char* s, size_type pos,
                             size_type count) const {
    return find_last_not_of(string_piece(s, count), pos);
  }

  // Overload of `string_piece::find_last_not_of()` for finding a different
  // C-style string `s` within the `string_piece`.
  size_type find_last_not_of(const char* s, size_type pos = npos) const {
    return find_last_not_of(string_piece(s), pos);
  }

  constexpr int32_t category() const noexcept {
    return category_;
  }

 private:
  // The constructor from std::string delegates to this constructor.
  // See the comment on that constructor for the rationale.
  struct SkipCheckLengthTag {};
  string_piece(const char* data, size_type len, SkipCheckLengthTag) noexcept
      : ptr_(data), length_(len) {}

  static constexpr size_type kMaxSize =
      (std::numeric_limits<difference_type>::max)();

  static constexpr size_type CheckLengthInternal(size_type len) {
    return TURBO_HARDENING_ASSERT(len <= kMaxSize), len;
  }

  static constexpr size_type StrlenInternal(const char* str) {
#if defined(_MSC_VER) && _MSC_VER >= 1910 && !defined(__clang__)
    // MSVC 2017+ can evaluate this at compile-time.
    const char* begin = str;
    while (*str != '\0') ++str;
    return str - begin;
#elif TURBO_HAVE_BUILTIN(__builtin_strlen) || \
    (defined(__GNUC__) && !defined(__clang__))
    // GCC has __builtin_strlen according to
    // https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Other-Builtins.html, but
    // TURBO_HAVE_BUILTIN doesn't detect that, so we use the extra checks above.
    // __builtin_strlen is constexpr.
    return __builtin_strlen(str);
#else
    return str ? strlen(str) : 0;
#endif
  }

  static constexpr size_t Min(size_type length_a, size_type length_b) {
    return length_a < length_b ? length_a : length_b;
  }

  static constexpr int CompareImpl(size_type length_a, size_type length_b,
                                   int compare_result) {
    return compare_result == 0 ? static_cast<int>(length_a > length_b) -
                                     static_cast<int>(length_a < length_b)
                               : (compare_result < 0 ? -1 : 1);
  }

  const char* ptr_;
  size_type length_;
  int32_t   category_;
};

// This large function is defined inline so that in a fairly common case where
// one of the arguments is a literal, the compiler can elide a lot of the
// following comparisons.
constexpr bool operator==(string_piece x, string_piece y) noexcept {
  return x.size() == y.size() &&
         (x.empty() ||
          TURBO_INTERNAL_STRING_VIEW_MEMCMP(x.data(), y.data(), x.size()) == 0);
}

constexpr bool operator!=(string_piece x, string_piece y) noexcept {
  return !(x == y);
}

constexpr bool operator<(string_piece x, string_piece y) noexcept {
  return x.compare(y) < 0;
}

constexpr bool operator>(string_piece x, string_piece y) noexcept {
  return y < x;
}

constexpr bool operator<=(string_piece x, string_piece y) noexcept {
  return !(y < x);
}

constexpr bool operator>=(string_piece x, string_piece y) noexcept {
  return !(x < y);
}

// IO Insertion Operator
std::ostream& operator<<(std::ostream& o, string_piece piece);

TURBO_NAMESPACE_END
}  // namespace turbo

#undef TURBO_INTERNAL_STRING_VIEW_MEMCMP

namespace turbo {
TURBO_NAMESPACE_BEGIN

// ClippedSubstr()
//
// Like `s.substr(pos, n)`, but clips `pos` to an upper bound of `s.size()`.
// Provided because turbo::string_piece::substr throws if `pos > size()`
inline string_piece ClippedSubstr(string_piece s, size_t pos,
                                 size_t n = string_piece::npos) {
  pos = (std::min)(pos, static_cast<size_t>(s.size()));
  return s.substr(pos, n);
}

// NullSafeStringView()
//
// Creates an `turbo::string_piece` from a pointer `p` even if it's null-valued.
// This function should be used where an `turbo::string_piece` can be created from
// a possibly-null pointer.
constexpr string_piece NullSafeStringView(const char* p) {
  return p ? string_piece(p) : string_piece();
}

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_STRING_PIECE_H_
