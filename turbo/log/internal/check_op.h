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
// File: log/internal/check_op.h
// -----------------------------------------------------------------------------
//
// This file declares helpers routines and macros used to implement `CHECK`
// macros.

#ifndef TURBO_LOG_INTERNAL_CHECK_OP_H_
#define TURBO_LOG_INTERNAL_CHECK_OP_H_

#include <stdint.h>

#include <cstddef>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/optimization.h>
#include <turbo/log/internal/nullguard.h>
#include <turbo/log/internal/nullstream.h>
#include <turbo/log/internal/strip.h>
#include <turbo/strings/has_stringify.h>
#include <turbo/strings/string_view.h>

// `TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL` wraps string literals that
// should be stripped when `TURBO_MIN_LOG_LEVEL` exceeds `kFatal`.
#ifdef TURBO_MIN_LOG_LEVEL
#define TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL(literal)         \
  (::turbo::LogSeverity::kFatal >=                               \
           static_cast<::turbo::LogSeverity>(TURBO_MIN_LOG_LEVEL) \
       ? (literal)                                              \
       : "")
#else
#define TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL(literal) (literal)
#endif

#ifdef NDEBUG
// `NDEBUG` is defined, so `DCHECK_EQ(x, y)` and so on do nothing.  However, we
// still want the compiler to parse `x` and `y`, because we don't want to lose
// potentially useful errors and warnings.
#define TURBO_LOG_INTERNAL_DCHECK_NOP(x, y)   \
  while (false && ((void)(x), (void)(y), 0)) \
  ::turbo::log_internal::NullStream().InternalStream()
#endif

#define TURBO_LOG_INTERNAL_CHECK_OP(name, op, val1, val1_text, val2, val2_text) \
  while (::std::string* turbo_log_internal_check_op_result                      \
             TURBO_LOG_INTERNAL_ATTRIBUTE_UNUSED_IF_STRIP_LOG =                 \
                 ::turbo::log_internal::name##Impl(                             \
                     ::turbo::log_internal::GetReferenceableValue(val1),        \
                     ::turbo::log_internal::GetReferenceableValue(val2),        \
                     TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL(                   \
                         val1_text " " #op " " val2_text)))                    \
    TURBO_LOG_INTERNAL_CONDITION_FATAL(STATELESS, true)                         \
  TURBO_LOG_INTERNAL_CHECK(*turbo_log_internal_check_op_result).InternalStream()
#define TURBO_LOG_INTERNAL_QCHECK_OP(name, op, val1, val1_text, val2, \
                                    val2_text)                       \
  while (::std::string* turbo_log_internal_qcheck_op_result =         \
             ::turbo::log_internal::name##Impl(                       \
                 ::turbo::log_internal::GetReferenceableValue(val1),  \
                 ::turbo::log_internal::GetReferenceableValue(val2),  \
                 TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL(             \
                     val1_text " " #op " " val2_text)))              \
    TURBO_LOG_INTERNAL_CONDITION_QFATAL(STATELESS, true)              \
  TURBO_LOG_INTERNAL_QCHECK(*turbo_log_internal_qcheck_op_result).InternalStream()
#define TURBO_LOG_INTERNAL_CHECK_STROP(func, op, expected, s1, s1_text, s2,     \
                                      s2_text)                                 \
  while (::std::string* turbo_log_internal_check_strop_result =                 \
             ::turbo::log_internal::Check##func##expected##Impl(                \
                 (s1), (s2),                                                   \
                 TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL(s1_text " " #op        \
                                                                " " s2_text))) \
    TURBO_LOG_INTERNAL_CONDITION_FATAL(STATELESS, true)                         \
  TURBO_LOG_INTERNAL_CHECK(*turbo_log_internal_check_strop_result)               \
      .InternalStream()
#define TURBO_LOG_INTERNAL_QCHECK_STROP(func, op, expected, s1, s1_text, s2,    \
                                       s2_text)                                \
  while (::std::string* turbo_log_internal_qcheck_strop_result =                \
             ::turbo::log_internal::Check##func##expected##Impl(                \
                 (s1), (s2),                                                   \
                 TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL(s1_text " " #op        \
                                                                " " s2_text))) \
    TURBO_LOG_INTERNAL_CONDITION_QFATAL(STATELESS, true)                        \
  TURBO_LOG_INTERNAL_QCHECK(*turbo_log_internal_qcheck_strop_result)             \
      .InternalStream()
// This one is tricky:
// * We must evaluate `val` exactly once, yet we need to do two things with it:
//   evaluate `.ok()` and (sometimes) `.ToString()`.
// * `val` might be an `turbo::Status` or some `turbo::StatusOr<T>`.
// * `val` might be e.g. `ATemporary().GetStatus()`, which may return a
//   reference to a member of `ATemporary` that is only valid until the end of
//   the full expression.
// * We don't want this file to depend on `turbo::Status` `#include`s or linkage,
//   nor do we want to move the definition to status and introduce a dependency
//   in the other direction.  We can be assured that callers must already have a
//   `Status` and the necessary `#include`s and linkage.
// * Callsites should be small and fast (at least when `val.ok()`): one branch,
//   minimal stack footprint.
//   * In particular, the string concat stuff should be out-of-line and emitted
//     in only one TU to save linker input size
// * We want the `val.ok()` check inline so static analyzers and optimizers can
//   see it.
// * As usual, no braces so we can stream into the expansion with `operator<<`.
// * Also as usual, it must expand to a single (partial) statement with no
//   ambiguous-else problems.
// * When stripped by `TURBO_MIN_LOG_LEVEL`, we must discard the `<expr> is OK`
//   string literal and abort without doing any streaming.  We don't need to
//   strip the call to stringify the non-ok `Status` as long as we don't log it;
//   dropping the `Status`'s message text is out of scope.
#define TURBO_LOG_INTERNAL_CHECK_OK(val, val_text)                        \
  for (::std::pair<const ::turbo::Status*, ::std::string*>                \
           turbo_log_internal_check_ok_goo;                               \
       turbo_log_internal_check_ok_goo.first =                            \
           ::turbo::log_internal::AsStatus(val),                          \
       turbo_log_internal_check_ok_goo.second =                           \
           TURBO_PREDICT_TRUE(turbo_log_internal_check_ok_goo.first->ok()) \
               ? nullptr                                                 \
               : ::turbo::status_internal::MakeCheckFailString(           \
                     turbo_log_internal_check_ok_goo.first,               \
                     TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL(val_text     \
                                                            " is OK")),  \
       !TURBO_PREDICT_TRUE(turbo_log_internal_check_ok_goo.first->ok());)  \
    TURBO_LOG_INTERNAL_CONDITION_FATAL(STATELESS, true)                   \
  TURBO_LOG_INTERNAL_CHECK(*turbo_log_internal_check_ok_goo.second)        \
      .InternalStream()
#define TURBO_LOG_INTERNAL_QCHECK_OK(val, val_text)                        \
  for (::std::pair<const ::turbo::Status*, ::std::string*>                 \
           turbo_log_internal_qcheck_ok_goo;                               \
       turbo_log_internal_qcheck_ok_goo.first =                            \
           ::turbo::log_internal::AsStatus(val),                           \
       turbo_log_internal_qcheck_ok_goo.second =                           \
           TURBO_PREDICT_TRUE(turbo_log_internal_qcheck_ok_goo.first->ok()) \
               ? nullptr                                                  \
               : ::turbo::status_internal::MakeCheckFailString(            \
                     turbo_log_internal_qcheck_ok_goo.first,               \
                     TURBO_LOG_INTERNAL_STRIP_STRING_LITERAL(val_text      \
                                                            " is OK")),   \
       !TURBO_PREDICT_TRUE(turbo_log_internal_qcheck_ok_goo.first->ok());)  \
    TURBO_LOG_INTERNAL_CONDITION_QFATAL(STATELESS, true)                   \
  TURBO_LOG_INTERNAL_QCHECK(*turbo_log_internal_qcheck_ok_goo.second)       \
      .InternalStream()

namespace turbo {
TURBO_NAMESPACE_BEGIN

class Status;
template <typename T>
class StatusOr;

namespace status_internal {
TURBO_ATTRIBUTE_PURE_FUNCTION std::string* MakeCheckFailString(
    const turbo::Status* status, const char* prefix);
}  // namespace status_internal

namespace log_internal {

// Convert a Status or a StatusOr to its underlying status value.
//
// (This implementation does not require a dep on turbo::Status to work.)
inline const turbo::Status* AsStatus(const turbo::Status& s) { return &s; }
template <typename T>
const turbo::Status* AsStatus(const turbo::StatusOr<T>& s) {
  return &s.status();
}

// A helper class for formatting `expr (V1 vs. V2)` in a `CHECK_XX` statement.
// See `MakeCheckOpString` for sample usage.
class CheckOpMessageBuilder final {
 public:
  // Inserts `exprtext` and ` (` to the stream.
  explicit CheckOpMessageBuilder(const char* exprtext);
  ~CheckOpMessageBuilder() = default;
  // For inserting the first variable.
  std::ostream& ForVar1() { return stream_; }
  // For inserting the second variable (adds an intermediate ` vs. `).
  std::ostream& ForVar2();
  // Get the result (inserts the closing `)`).
  std::string* NewString();

 private:
  std::ostringstream stream_;
};

// This formats a value for a failing `CHECK_XX` statement.  Ordinarily, it uses
// the definition for `operator<<`, with a few special cases below.
template <typename T>
inline void MakeCheckOpValueString(std::ostream& os, const T& v) {
  os << log_internal::NullGuard<T>::Guard(v);
}

// Overloads for char types provide readable values for unprintable characters.
void MakeCheckOpValueString(std::ostream& os, char v);
void MakeCheckOpValueString(std::ostream& os, signed char v);
void MakeCheckOpValueString(std::ostream& os, unsigned char v);
void MakeCheckOpValueString(std::ostream& os, const void* p);

namespace detect_specialization {

// MakeCheckOpString is being specialized for every T and U pair that is being
// passed to the CHECK_op macros. However, there is a lot of redundancy in these
// specializations that creates unnecessary library and binary bloat.
// The number of instantiations tends to be O(n^2) because we have two
// independent inputs. This technique works by reducing `n`.
//
// Most user-defined types being passed to CHECK_op end up being printed as a
// builtin type. For example, enums tend to be implicitly converted to its
// underlying type when calling operator<<, and pointers are printed with the
// `const void*` overload.
// To reduce the number of instantiations we coerce these values before calling
// MakeCheckOpString instead of inside it.
//
// To detect if this coercion is needed, we duplicate all the relevant
// operator<< overloads as specified in the standard, just in a different
// namespace. If the call to `stream << value` becomes ambiguous, it means that
// one of these overloads is the one selected by overload resolution. We then
// do overload resolution again just with our overload set to see which one gets
// selected. That tells us which type to coerce to.
// If the augmented call was not ambiguous, it means that none of these were
// selected and we can't coerce the input.
//
// As a secondary step to reduce code duplication, we promote integral types to
// their 64-bit variant. This does not change the printed value, but reduces the
// number of instantiations even further. Promoting an integer is very cheap at
// the call site.
int64_t operator<<(std::ostream&, short value);           // NOLINT
int64_t operator<<(std::ostream&, unsigned short value);  // NOLINT
int64_t operator<<(std::ostream&, int value);
int64_t operator<<(std::ostream&, unsigned int value);
int64_t operator<<(std::ostream&, long value);                 // NOLINT
uint64_t operator<<(std::ostream&, unsigned long value);       // NOLINT
int64_t operator<<(std::ostream&, long long value);            // NOLINT
uint64_t operator<<(std::ostream&, unsigned long long value);  // NOLINT
float operator<<(std::ostream&, float value);
double operator<<(std::ostream&, double value);
long double operator<<(std::ostream&, long double value);
bool operator<<(std::ostream&, bool value);
const void* operator<<(std::ostream&, const void* value);
const void* operator<<(std::ostream&, std::nullptr_t);

// These `char` overloads are specified like this in the standard, so we have to
// write them exactly the same to ensure the call is ambiguous.
// If we wrote it in a different way (eg taking std::ostream instead of the
// template) then one call might have a higher rank than the other and it would
// not be ambiguous.
template <typename Traits>
char operator<<(std::basic_ostream<char, Traits>&, char);
template <typename Traits>
signed char operator<<(std::basic_ostream<char, Traits>&, signed char);
template <typename Traits>
unsigned char operator<<(std::basic_ostream<char, Traits>&, unsigned char);
template <typename Traits>
const char* operator<<(std::basic_ostream<char, Traits>&, const char*);
template <typename Traits>
const signed char* operator<<(std::basic_ostream<char, Traits>&,
                              const signed char*);
template <typename Traits>
const unsigned char* operator<<(std::basic_ostream<char, Traits>&,
                                const unsigned char*);

// This overload triggers when the call is not ambiguous.
// It means that T is being printed with some overload not on this list.
// We keep the value as `const T&`.
template <typename T, typename = decltype(std::declval<std::ostream&>()
                                          << std::declval<const T&>())>
const T& Detect(int);

// This overload triggers when the call is ambiguous.
// It means that T is either one from this list or printed as one from this
// list. Eg an enum that decays to `int` for printing.
// We ask the overload set to give us the type we want to convert it to.
template <typename T>
decltype(detect_specialization::operator<<(std::declval<std::ostream&>(),
                                           std::declval<const T&>()))
Detect(char);

// A sink for turbo_stringify which redirects everything to a std::ostream.
class StringifySink {
 public:
  explicit StringifySink(std::ostream& os TURBO_ATTRIBUTE_LIFETIME_BOUND);

  void Append(turbo::string_view text);
  void Append(size_t length, char ch);
  friend void TurboFormatFlush(StringifySink* sink, turbo::string_view text);

 private:
  std::ostream& os_;
};

// Wraps a type implementing turbo_stringify, and implements operator<<.
template <typename T>
class StringifyToStreamWrapper {
 public:
  explicit StringifyToStreamWrapper(const T& v TURBO_ATTRIBUTE_LIFETIME_BOUND)
      : v_(v) {}

  friend std::ostream& operator<<(std::ostream& os,
                                  const StringifyToStreamWrapper& wrapper) {
    StringifySink sink(os);
    turbo_stringify(sink, wrapper.v_);
    return os;
  }

 private:
  const T& v_;
};

// This overload triggers when T implements turbo_stringify.
// StringifyToStreamWrapper is used to allow MakeCheckOpString to use
// operator<<.
template <typename T>
std::enable_if_t<HasTurboStringify<T>::value,
                 StringifyToStreamWrapper<T>>
Detect(...);  // Ellipsis has lowest preference when int passed.
}  // namespace detect_specialization

template <typename T>
using CheckOpStreamType = decltype(detect_specialization::Detect<T>(0));

// Build the error message string.  Specify no inlining for code size.
template <typename T1, typename T2>
TURBO_ATTRIBUTE_RETURNS_NONNULL std::string* MakeCheckOpString(
    T1 v1, T2 v2, const char* exprtext) TURBO_ATTRIBUTE_NOINLINE;

template <typename T1, typename T2>
std::string* MakeCheckOpString(T1 v1, T2 v2, const char* exprtext) {
  CheckOpMessageBuilder comb(exprtext);
  MakeCheckOpValueString(comb.ForVar1(), v1);
  MakeCheckOpValueString(comb.ForVar2(), v2);
  return comb.NewString();
}

// Add a few commonly used instantiations as extern to reduce size of objects
// files.
#define TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(x) \
  extern template std::string* MakeCheckOpString(x, x, const char*)
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(bool);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(int64_t);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(uint64_t);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(float);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(double);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(char);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(unsigned char);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(const std::string&);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(const turbo::string_view&);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(const char*);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(const signed char*);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(const unsigned char*);
TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN(const void*);
#undef TURBO_LOG_INTERNAL_DEFINE_MAKE_CHECK_OP_STRING_EXTERN

// `TURBO_LOG_INTERNAL_CHECK_OP_IMPL_RESULT` skips formatting the Check_OP result
// string iff `TURBO_MIN_LOG_LEVEL` exceeds `kFatal`, instead returning an empty
// string.
#ifdef TURBO_MIN_LOG_LEVEL
#define TURBO_LOG_INTERNAL_CHECK_OP_IMPL_RESULT(U1, U2, v1, v2, exprtext) \
  ((::turbo::LogSeverity::kFatal >=                                       \
    static_cast<::turbo::LogSeverity>(TURBO_MIN_LOG_LEVEL))                \
       ? MakeCheckOpString<U1, U2>(v1, v2, exprtext)                     \
       : new std::string())
#else
#define TURBO_LOG_INTERNAL_CHECK_OP_IMPL_RESULT(U1, U2, v1, v2, exprtext) \
  MakeCheckOpString<U1, U2>(v1, v2, exprtext)
#endif

// Helper functions for `TURBO_LOG_INTERNAL_CHECK_OP` macro family.  The
// `(int, int)` override works around the issue that the compiler will not
// instantiate the template version of the function on values of unnamed enum
// type.
#define TURBO_LOG_INTERNAL_CHECK_OP_IMPL(name, op)                          \
  template <typename T1, typename T2>                                      \
  inline constexpr ::std::string* name##Impl(const T1& v1, const T2& v2,   \
                                             const char* exprtext) {       \
    using U1 = CheckOpStreamType<T1>;                                      \
    using U2 = CheckOpStreamType<T2>;                                      \
    return TURBO_PREDICT_TRUE(v1 op v2)                                     \
               ? nullptr                                                   \
               : TURBO_LOG_INTERNAL_CHECK_OP_IMPL_RESULT(U1, U2, U1(v1),    \
                                                        U2(v2), exprtext); \
  }                                                                        \
  inline constexpr ::std::string* name##Impl(int v1, int v2,               \
                                             const char* exprtext) {       \
    return name##Impl<int, int>(v1, v2, exprtext);                         \
  }

TURBO_LOG_INTERNAL_CHECK_OP_IMPL(Check_EQ, ==)
TURBO_LOG_INTERNAL_CHECK_OP_IMPL(Check_NE, !=)
TURBO_LOG_INTERNAL_CHECK_OP_IMPL(Check_LE, <=)
TURBO_LOG_INTERNAL_CHECK_OP_IMPL(Check_LT, <)
TURBO_LOG_INTERNAL_CHECK_OP_IMPL(Check_GE, >=)
TURBO_LOG_INTERNAL_CHECK_OP_IMPL(Check_GT, >)
#undef TURBO_LOG_INTERNAL_CHECK_OP_IMPL_RESULT
#undef TURBO_LOG_INTERNAL_CHECK_OP_IMPL

std::string* CheckstrcmptrueImpl(const char* s1, const char* s2,
                                 const char* exprtext);
std::string* CheckstrcmpfalseImpl(const char* s1, const char* s2,
                                  const char* exprtext);
std::string* CheckstrcasecmptrueImpl(const char* s1, const char* s2,
                                     const char* exprtext);
std::string* CheckstrcasecmpfalseImpl(const char* s1, const char* s2,
                                      const char* exprtext);

// `CHECK_EQ` and friends want to pass their arguments by reference, however
// this winds up exposing lots of cases where people have defined and
// initialized static const data members but never declared them (i.e. in a .cc
// file), meaning they are not referenceable.  This function avoids that problem
// for integers (the most common cases) by overloading for every primitive
// integer type, even the ones we discourage, and returning them by value.
template <typename T>
inline constexpr const T& GetReferenceableValue(const T& t) {
  return t;
}
inline constexpr char GetReferenceableValue(char t) { return t; }
inline constexpr unsigned char GetReferenceableValue(unsigned char t) {
  return t;
}
inline constexpr signed char GetReferenceableValue(signed char t) { return t; }
inline constexpr short GetReferenceableValue(short t) { return t; }  // NOLINT
inline constexpr unsigned short GetReferenceableValue(               // NOLINT
    unsigned short t) {                                              // NOLINT
  return t;
}
inline constexpr int GetReferenceableValue(int t) { return t; }
inline constexpr unsigned int GetReferenceableValue(unsigned int t) {
  return t;
}
inline constexpr long GetReferenceableValue(long t) { return t; }  // NOLINT
inline constexpr unsigned long GetReferenceableValue(              // NOLINT
    unsigned long t) {                                             // NOLINT
  return t;
}
inline constexpr long long GetReferenceableValue(long long t) {  // NOLINT
  return t;
}
inline constexpr unsigned long long GetReferenceableValue(  // NOLINT
    unsigned long long t) {                                 // NOLINT
  return t;
}

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_CHECK_OP_H_
