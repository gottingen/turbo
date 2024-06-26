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
// File: hash.h
// -----------------------------------------------------------------------------
//

#pragma once

#ifdef __APPLE__
#include <Availability.h>
#include <TargetConditionals.h>
#endif

#include <turbo/base/config.h>

// For feature testing and determining which headers can be included.
#if TURBO_INTERNAL_CPLUSPLUS_LANG >= 202002L
#include <version>
#else

#include <ciso646>

#endif

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <deque>
#include <forward_list>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <turbo/base/internal/unaligned_access.h>
#include <turbo/base/port.h>
#include <turbo/container/fixed_array.h>
#include <turbo/hash/internal/city.h>
#include <turbo/hash/internal/low_level_hash.h>
#include <turbo/meta/type_traits.h>
#include <turbo/numeric/bits.h>
#include <turbo/numeric/int128.h>
#include <turbo/strings/string_view.h>
#include <optional>
#include <variant>
#include <turbo/meta/utility.h>

#if defined(__cpp_lib_filesystem) && __cpp_lib_filesystem >= 201703L && \
    !defined(_LIBCPP_HAS_NO_FILESYSTEM_LIBRARY)
#include <filesystem>  // NOLINT
#endif


#include <string_view>

namespace turbo {
    TURBO_NAMESPACE_BEGIN

    class HashState;

    namespace hash_internal {

        // Internal detail: Large buffers are hashed in smaller chunks.  This function
        // returns the size of these chunks.
        constexpr size_t PiecewiseChunkSize() { return 1024; }

        // PiecewiseCombiner
        //
        // PiecewiseCombiner is an internal-only helper class for hashing a piecewise
        // buffer of `char` or `unsigned char` as though it were contiguous.  This class
        // provides two methods:
        //
        //   H add_buffer(state, data, size)
        //   H finalize(state)
        //
        // `add_buffer` can be called zero or more times, followed by a single call to
        // `finalize`.  This will produce the same hash expansion as concatenating each
        // buffer piece into a single contiguous buffer, and passing this to
        // `H::combine_contiguous`.
        //
        //  Example usage:
        //    PiecewiseCombiner combiner;
        //    for (const auto& piece : pieces) {
        //      state = combiner.add_buffer(std::move(state), piece.data, piece.size);
        //    }
        //    return combiner.finalize(std::move(state));
        class PiecewiseCombiner {
        public:
            PiecewiseCombiner() : position_(0) {}

            PiecewiseCombiner(const PiecewiseCombiner &) = delete;

            PiecewiseCombiner &operator=(const PiecewiseCombiner &) = delete;

            // PiecewiseCombiner::add_buffer()
            //
            // Appends the given range of bytes to the sequence to be hashed, which may
            // modify the provided hash state.
            template<typename H>
            H add_buffer(H state, const unsigned char *data, size_t size);

            template<typename H>
            H add_buffer(H state, const char *data, size_t size) {
                return add_buffer(std::move(state),
                                  reinterpret_cast<const unsigned char *>(data), size);
            }

            // PiecewiseCombiner::finalize()
            //
            // Finishes combining the hash sequence, which may may modify the provided
            // hash state.
            //
            // Once finalize() is called, add_buffer() may no longer be called. The
            // resulting hash state will be the same as if the pieces passed to
            // add_buffer() were concatenated into a single flat buffer, and then provided
            // to H::combine_contiguous().
            template<typename H>
            H finalize(H state);

        private:
            unsigned char buf_[PiecewiseChunkSize()];
            size_t position_;
        };

        // is_hashable()
        //
        // Trait class which returns true if T is hashable by the turbo::Hash framework.
        // Used for the turbo_hash_value implementations for composite types below.
        template<typename T>
        struct is_hashable;

        // HashStateBase
        //
        // An internal implementation detail that contains common implementation details
        // for all of the "hash state objects" objects generated by Turbo.  This is not
        // a public API; users should not create classes that inherit from this.
        //
        // A hash state object is the template argument `H` passed to `turbo_hash_value`.
        // It represents an intermediate state in the computation of an unspecified hash
        // algorithm. `HashStateBase` provides a CRTP style base class for hash state
        // implementations. Developers adding type support for `turbo::Hash` should not
        // rely on any parts of the state object other than the following member
        // functions:
        //
        //   * HashStateBase::combine()
        //   * HashStateBase::combine_contiguous()
        //   * HashStateBase::combine_unordered()
        //
        // A derived hash state class of type `H` must provide a public member function
        // with a signature similar to the following:
        //
        //    `static H combine_contiguous(H state, const unsigned char*, size_t)`.
        //
        // It must also provide a private template method named RunCombineUnordered.
        //
        // A "consumer" is a 1-arg functor returning void.  Its argument is a reference
        // to an inner hash state object, and it may be called multiple times.  When
        // called, the functor consumes the entropy from the provided state object,
        // and resets that object to its empty state.
        //
        // A "combiner" is a stateless 2-arg functor returning void.  Its arguments are
        // an inner hash state object and an ElementStateConsumer functor.  A combiner
        // uses the provided inner hash state object to hash each element of the
        // container, passing the inner hash state object to the consumer after hashing
        // each element.
        //
        // Given these definitions, a derived hash state class of type H
        // must provide a private template method with a signature similar to the
        // following:
        //
        //    `template <typename CombinerT>`
        //    `static H RunCombineUnordered(H outer_state, CombinerT combiner)`
        //
        // This function is responsible for constructing the inner state object and
        // providing a consumer to the combiner.  It uses side effects of the consumer
        // and combiner to mix the state of each element in an order-independent manner,
        // and uses this to return an updated value of `outer_state`.
        //
        // This inside-out approach generates efficient object code in the normal case,
        // but allows us to use stack storage to implement the turbo::HashState type
        // erasure mechanism (avoiding heap allocations while hashing).
        //
        // `HashStateBase` will provide a complete implementation for a hash state
        // object in terms of these two methods.
        //
        // Example:
        //
        //   // Use CRTP to define your derived class.
        //   struct MyHashState : HashStateBase<MyHashState> {
        //       static H combine_contiguous(H state, const unsigned char*, size_t);
        //       using MyHashState::HashStateBase::combine;
        //       using MyHashState::HashStateBase::combine_contiguous;
        //       using MyHashState::HashStateBase::combine_unordered;
        //     private:
        //       template <typename CombinerT>
        //       static H RunCombineUnordered(H state, CombinerT combiner);
        //   };
        template<typename H>
        class HashStateBase {
        public:
            // HashStateBase::combine()
            //
            // Combines an arbitrary number of values into a hash state, returning the
            // updated state.
            //
            // Each of the value types `T` must be separately hashable by the Turbo
            // hashing framework.
            //
            // NOTE:
            //
            //   state = H::combine(std::move(state), value1, value2, value3);
            //
            // is guaranteed to produce the same hash expansion as:
            //
            //   state = H::combine(std::move(state), value1);
            //   state = H::combine(std::move(state), value2);
            //   state = H::combine(std::move(state), value3);
            template<typename T, typename... Ts>
            static H combine(H state, const T &value, const Ts &... values);

            static H combine(H state) { return state; }

            // HashStateBase::combine_contiguous()
            //
            // Combines a contiguous array of `size` elements into a hash state, returning
            // the updated state.
            //
            // NOTE:
            //
            //   state = H::combine_contiguous(std::move(state), data, size);
            //
            // is NOT guaranteed to produce the same hash expansion as a for-loop (it may
            // perform internal optimizations).  If you need this guarantee, use the
            // for-loop instead.
            template<typename T>
            static H combine_contiguous(H state, const T *data, size_t size);

            template<typename I>
            static H combine_unordered(H state, I begin, I end);

            using TurboInternalPiecewiseCombiner = PiecewiseCombiner;

            template<typename T>
            using is_hashable = turbo::hash_internal::is_hashable<T>;

        private:
            // Common implementation of the iteration step of a "combiner", as described
            // above.
            template<typename I>
            struct CombineUnorderedCallback {
                I begin;
                I end;

                template<typename InnerH, typename ElementStateConsumer>
                void operator()(InnerH inner_state, ElementStateConsumer cb) {
                    for (; begin != end; ++begin) {
                        inner_state = H::combine(std::move(inner_state), *begin);
                        cb(inner_state);
                    }
                }
            };
        };

        // is_uniquely_represented
        //
        // `is_uniquely_represented<T>` is a trait class that indicates whether `T`
        // is uniquely represented.
        //
        // A type is "uniquely represented" if two equal values of that type are
        // guaranteed to have the same bytes in their underlying storage. In other
        // words, if `a == b`, then `memcmp(&a, &b, sizeof(T))` is guaranteed to be
        // zero. This property cannot be detected automatically, so this trait is false
        // by default, but can be specialized by types that wish to assert that they are
        // uniquely represented. This makes them eligible for certain optimizations.
        //
        // If you have any doubt whatsoever, do not specialize this template.
        // The default is completely safe, and merely disables some optimizations
        // that will not matter for most types. Specializing this template,
        // on the other hand, can be very hazardous.
        //
        // To be uniquely represented, a type must not have multiple ways of
        // representing the same value; for example, float and double are not
        // uniquely represented, because they have distinct representations for
        // +0 and -0. Furthermore, the type's byte representation must consist
        // solely of user-controlled data, with no padding bits and no compiler-
        // controlled data such as vptrs or sanitizer metadata. This is usually
        // very difficult to guarantee, because in most cases the compiler can
        // insert data and padding bits at its own discretion.
        //
        // If you specialize this template for a type `T`, you must do so in the file
        // that defines that type (or in this file). If you define that specialization
        // anywhere else, `is_uniquely_represented<T>` could have different meanings
        // in different places.
        //
        // The Enable parameter is meaningless; it is provided as a convenience,
        // to support certain SFINAE techniques when defining specializations.
        template<typename T, typename Enable = void>
        struct is_uniquely_represented : std::false_type {
        };

        // is_uniquely_represented<unsigned char>
        //
        // unsigned char is a synonym for "byte", so it is guaranteed to be
        // uniquely represented.
        template<>
        struct is_uniquely_represented<unsigned char> : std::true_type {
        };

        // is_uniquely_represented for non-standard integral types
        //
        // Integral types other than bool should be uniquely represented on any
        // platform that this will plausibly be ported to.
        template<typename Integral>
        struct is_uniquely_represented<
                Integral, typename std::enable_if<std::is_integral<Integral>::value>::type>
                : std::true_type {
        };

        // is_uniquely_represented<bool>
        //
        //
        template<>
        struct is_uniquely_represented<bool> : std::false_type {
        };

        // hash_bytes()
        //
        // Convenience function that combines `hash_state` with the byte representation
        // of `value`.
        template<typename H, typename T>
        H hash_bytes(H hash_state, const T &value) {
            const unsigned char *start = reinterpret_cast<const unsigned char *>(&value);
            return H::combine_contiguous(std::move(hash_state), start, sizeof(value));
        }

        // -----------------------------------------------------------------------------
        // turbo_hash_value for Basic Types
        // -----------------------------------------------------------------------------

        // Note: Default `turbo_hash_value` implementations live in `hash_internal`. This
        // allows us to block lexical scope lookup when doing an unqualified call to
        // `turbo_hash_value` below. User-defined implementations of `turbo_hash_value` can
        // only be found via ADL.

        // turbo_hash_value() for hashing bool values
        //
        // We use SFINAE to ensure that this overload only accepts bool, not types that
        // are convertible to bool.
        template<typename H, typename B>
        typename std::enable_if<std::is_same<B, bool>::value, H>::type turbo_hash_value(
                H hash_state, B value) {
            return H::combine(std::move(hash_state),
                              static_cast<unsigned char>(value ? 1 : 0));
        }

        // turbo_hash_value() for hashing enum values
        template<typename H, typename Enum>
        typename std::enable_if<std::is_enum<Enum>::value, H>::type turbo_hash_value(
                H hash_state, Enum e) {
            // In practice, we could almost certainly just invoke hash_bytes directly,
            // but it's possible that a sanitizer might one day want to
            // store data in the unused bits of an enum. To avoid that risk, we
            // convert to the underlying type before hashing. Hopefully this will get
            // optimized away; if not, we can reopen discussion with c-toolchain-team.
            return H::combine(std::move(hash_state),
                              static_cast<typename std::underlying_type<Enum>::type>(e));
        }

        // turbo_hash_value() for hashing floating-point values
        template<typename H, typename Float>
        typename std::enable_if<std::is_same<Float, float>::value ||
                                std::is_same<Float, double>::value,
                H>::type
        turbo_hash_value(H hash_state, Float value) {
            return hash_internal::hash_bytes(std::move(hash_state),
                                             value == 0 ? 0 : value);
        }

        // Long double has the property that it might have extra unused bytes in it.
        // For example, in x86 sizeof(long double)==16 but it only really uses 80-bits
        // of it. This means we can't use hash_bytes on a long double and have to
        // convert it to something else first.
        template<typename H, typename LongDouble>
        typename std::enable_if<std::is_same<LongDouble, long double>::value, H>::type
        turbo_hash_value(H hash_state, LongDouble value) {
            const int category = std::fpclassify(value);
            switch (category) {
                case FP_INFINITE:
                    // Add the sign bit to differentiate between +Inf and -Inf
                    hash_state = H::combine(std::move(hash_state), std::signbit(value));
                    break;

                case FP_NAN:
                case FP_ZERO:
                default:
                    // Category is enough for these.
                    break;

                case FP_NORMAL:
                case FP_SUBNORMAL:
                    // We can't convert `value` directly to double because this would have
                    // undefined behavior if the value is out of range.
                    // std::frexp gives us a value in the range (-1, -.5] or [.5, 1) that is
                    // guaranteed to be in range for `double`. The truncation is
                    // implementation defined, but that works as long as it is deterministic.
                    int exp;
                    auto mantissa = static_cast<double>(std::frexp(value, &exp));
                    hash_state = H::combine(std::move(hash_state), mantissa, exp);
            }

            return H::combine(std::move(hash_state), category);
        }

        // Without this overload, an array decays to a pointer and we hash that, which
        // is not likely to be what the caller intended.
        template<typename H, typename T, size_t N>
        H turbo_hash_value(H hash_state, T (&)[N]) {
            static_assert(
                    sizeof(T) == -1,
                    "Hashing C arrays is not allowed. For string literals, wrap the literal "
                    "in std::string_view(). To hash the array contents, use "
                    "turbo::MakeSpan() or make the array an std::array. To hash the array "
                    "address, use &array[0].");
            return hash_state;
        }

        // turbo_hash_value() for hashing pointers
        template<typename H, typename T>
        std::enable_if_t<std::is_pointer<T>::value, H> turbo_hash_value(H hash_state,
                                                                        T ptr) {
            auto v = reinterpret_cast<uintptr_t>(ptr);
            // Due to alignment, pointers tend to have low bits as zero, and the next few
            // bits follow a pattern since they are also multiples of some base value.
            // Mixing the pointer twice helps prevent stuck low bits for certain alignment
            // values.
            return H::combine(std::move(hash_state), v, v);
        }

        // turbo_hash_value() for hashing nullptr_t
        template<typename H>
        H turbo_hash_value(H hash_state, std::nullptr_t) {
            return H::combine(std::move(hash_state), static_cast<void *>(nullptr));
        }

        // turbo_hash_value() for hashing pointers-to-member
        template<typename H, typename T, typename C>
        H turbo_hash_value(H hash_state, T C::*ptr) {
            auto salient_ptm_size = [](std::size_t n) -> std::size_t {
#if defined(_MSC_VER)
                // Pointers-to-member-function on MSVC consist of one pointer plus 0, 1, 2,
                // or 3 ints. In 64-bit mode, they are 8-byte aligned and thus can contain
                // padding (namely when they have 1 or 3 ints). The value below is a lower
                // bound on the number of salient, non-padding bytes that we use for
                // hashing.
                if (alignof(T C::*) == alignof(int)) {
                  // No padding when all subobjects have the same size as the total
                  // alignment. This happens in 32-bit mode.
                  return n;
                } else {
                  // Padding for 1 int (size 16) or 3 ints (size 24).
                  // With 2 ints, the size is 16 with no padding, which we pessimize.
                  return n == 24 ? 20 : n == 16 ? 12 : n;
                }
#else
                // On other platforms, we assume that pointers-to-members do not have
                // padding.
#ifdef __cpp_lib_has_unique_object_representations
                static_assert(std::has_unique_object_representations<T C::*>::value);
#endif  // __cpp_lib_has_unique_object_representations
                return n;
#endif
            };
            return H::combine_contiguous(std::move(hash_state),
                                         reinterpret_cast<unsigned char *>(&ptr),
                                         salient_ptm_size(sizeof ptr));
        }

        // -----------------------------------------------------------------------------
        // turbo_hash_value for Composite Types
        // -----------------------------------------------------------------------------

        // turbo_hash_value() for hashing pairs
        template<typename H, typename T1, typename T2>
        typename std::enable_if<is_hashable<T1>::value && is_hashable<T2>::value,
                H>::type
        turbo_hash_value(H hash_state, const std::pair<T1, T2> &p) {
            return H::combine(std::move(hash_state), p.first, p.second);
        }

        // hash_tuple()
        //
        // Helper function for hashing a tuple. The third argument should
        // be an index_sequence running from 0 to tuple_size<Tuple> - 1.
        template<typename H, typename Tuple, size_t... Is>
        H hash_tuple(H hash_state, const Tuple &t, turbo::index_sequence<Is...>) {
            return H::combine(std::move(hash_state), std::get<Is>(t)...);
        }

        // turbo_hash_value for hashing tuples
        template<typename H, typename... Ts>
#if defined(_MSC_VER)
        // This SFINAE gets MSVC confused under some conditions. Let's just disable it
        // for now.
        H
#else   // _MSC_VER
        typename std::enable_if<turbo::conjunction<is_hashable<Ts>...>::value, H>::type
#endif  // _MSC_VER
        turbo_hash_value(H hash_state, const std::tuple<Ts...> &t) {
            return hash_internal::hash_tuple(std::move(hash_state), t,
                                             turbo::make_index_sequence<sizeof...(Ts)>());
        }

        // -----------------------------------------------------------------------------
        // turbo_hash_value for Pointers
        // -----------------------------------------------------------------------------

        // turbo_hash_value for hashing unique_ptr
        template<typename H, typename T, typename D>
        H turbo_hash_value(H hash_state, const std::unique_ptr<T, D> &ptr) {
            return H::combine(std::move(hash_state), ptr.get());
        }

        // turbo_hash_value for hashing shared_ptr
        template<typename H, typename T>
        H turbo_hash_value(H hash_state, const std::shared_ptr<T> &ptr) {
            return H::combine(std::move(hash_state), ptr.get());
        }

        // -----------------------------------------------------------------------------
        // turbo_hash_value for String-Like Types
        // -----------------------------------------------------------------------------

        // turbo_hash_value for hashing strings
        //
        // All the string-like types supported here provide the same hash expansion for
        // the same character sequence. These types are:
        //
        //  - `turbo::Cord`
        //  - `std::string` (and std::basic_string<T, std::char_traits<T>, A> for
        //      any allocator A and any T in {char, wchar_t, char16_t, char32_t})
        //  - `std::string_view`, `std::string_view`, `std::wstring_view`,
        //    `std::u16string_view`, and `std::u32_string_view`.
        //
        // For simplicity, we currently support only strings built on `char`, `wchar_t`,
        // `char16_t`, or `char32_t`. This support may be broadened, if necessary, but
        // with some caution - this overload would misbehave in cases where the traits'
        // `eq()` member isn't equivalent to `==` on the underlying character type.
        template<typename H>
        H turbo_hash_value(H hash_state, std::string_view str) {
            return H::combine(
                    H::combine_contiguous(std::move(hash_state), str.data(), str.size()),
                    str.size());
        }

        // Support std::wstring, std::u16string and std::u32string.
        template<typename Char, typename Alloc, typename H,
                typename = turbo::enable_if_t<std::is_same<Char, wchar_t>::value ||
                                              std::is_same<Char, char16_t>::value ||
                                              std::is_same<Char, char32_t>::value>>
        H turbo_hash_value(
                H hash_state,
                const std::basic_string<Char, std::char_traits<Char>, Alloc> &str) {
            return H::combine(
                    H::combine_contiguous(std::move(hash_state), str.data(), str.size()),
                    str.size());
        }


        // Support std::wstring_view, std::u16string_view and std::u32string_view.
        template<typename Char, typename H,
                typename = turbo::enable_if_t<std::is_same<Char, wchar_t>::value ||
                                              std::is_same<Char, char16_t>::value ||
                                              std::is_same<Char, char32_t>::value>>
        H turbo_hash_value(H hash_state, std::basic_string_view<Char> str) {
            return H::combine(
                    H::combine_contiguous(std::move(hash_state), str.data(), str.size()),
                    str.size());
        }


#if defined(__cpp_lib_filesystem) && __cpp_lib_filesystem >= 201703L && \
    !defined(_LIBCPP_HAS_NO_FILESYSTEM_LIBRARY) && \
    (!defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || \
     __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ >= 130000)

#define TURBO_INTERNAL_STD_FILESYSTEM_PATH_HASH_AVAILABLE 1

        // Support std::filesystem::path. The SFINAE is required because some string
        // types are implicitly convertible to std::filesystem::path.
        template <typename Path, typename H,
                  typename = turbo::enable_if_t<
                      std::is_same_v<Path, std::filesystem::path>>>
        H turbo_hash_value(H hash_state, const Path& path) {
          // This is implemented by deferring to the standard library to compute the
          // hash.  The standard library requires that for two paths, `p1 == p2`, then
          // `hash_value(p1) == hash_value(p2)`. `turbo_hash_value` has the same
          // requirement. Since `operator==` does platform specific matching, deferring
          // to the standard library is the simplest approach.
          return H::combine(std::move(hash_state), std::filesystem::hash_value(path));
        }

#endif  // TURBO_INTERNAL_STD_FILESYSTEM_PATH_HASH_AVAILABLE

        // -----------------------------------------------------------------------------
        // turbo_hash_value for Sequence Containers
        // -----------------------------------------------------------------------------

        // turbo_hash_value for hashing std::array
        template<typename H, typename T, size_t N>
        typename std::enable_if<is_hashable<T>::value, H>::type turbo_hash_value(
                H hash_state, const std::array<T, N> &array) {
            return H::combine_contiguous(std::move(hash_state), array.data(),
                                         array.size());
        }

        // turbo_hash_value for hashing std::deque
        template<typename H, typename T, typename Allocator>
        typename std::enable_if<is_hashable<T>::value, H>::type turbo_hash_value(
                H hash_state, const std::deque<T, Allocator> &deque) {
            // TODO(gromer): investigate a more efficient implementation taking
            // advantage of the chunk structure.
            for (const auto &t: deque) {
                hash_state = H::combine(std::move(hash_state), t);
            }
            return H::combine(std::move(hash_state), deque.size());
        }

        // turbo_hash_value for hashing std::forward_list
        template<typename H, typename T, typename Allocator>
        typename std::enable_if<is_hashable<T>::value, H>::type turbo_hash_value(
                H hash_state, const std::forward_list<T, Allocator> &list) {
            size_t size = 0;
            for (const T &t: list) {
                hash_state = H::combine(std::move(hash_state), t);
                ++size;
            }
            return H::combine(std::move(hash_state), size);
        }

        // turbo_hash_value for hashing std::list
        template<typename H, typename T, typename Allocator>
        typename std::enable_if<is_hashable<T>::value, H>::type turbo_hash_value(
                H hash_state, const std::list<T, Allocator> &list) {
            for (const auto &t: list) {
                hash_state = H::combine(std::move(hash_state), t);
            }
            return H::combine(std::move(hash_state), list.size());
        }

        // turbo_hash_value for hashing std::vector
        //
        // Do not use this for vector<bool> on platforms that have a working
        // implementation of std::hash. It does not have a .data(), and a fallback for
        // std::hash<> is most likely faster.
        template<typename H, typename T, typename Allocator>
        typename std::enable_if<is_hashable<T>::value && !std::is_same<T, bool>::value,
                H>::type
        turbo_hash_value(H hash_state, const std::vector<T, Allocator> &vector) {
            return H::combine(H::combine_contiguous(std::move(hash_state), vector.data(),
                                                    vector.size()),
                              vector.size());
        }

// turbo_hash_value special cases for hashing std::vector<bool>

#if defined(TURBO_IS_BIG_ENDIAN) && \
    (defined(__GLIBCXX__) || defined(__GLIBCPP__))

        // std::hash in libstdc++ does not work correctly with vector<bool> on Big
        // Endian platforms therefore we need to implement a custom turbo_hash_value for
        // it. More details on the bug:
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102531
        template <typename H, typename T, typename Allocator>
        typename std::enable_if<is_hashable<T>::value && std::is_same<T, bool>::value,
                                H>::type
        turbo_hash_value(H hash_state, const std::vector<T, Allocator>& vector) {
          typename H::TurboInternalPiecewiseCombiner combiner;
          for (const auto& i : vector) {
            unsigned char c = static_cast<unsigned char>(i);
            hash_state = combiner.add_buffer(std::move(hash_state), &c, sizeof(c));
          }
          return H::combine(combiner.finalize(std::move(hash_state)), vector.size());
        }
#else

        // When not working around the libstdc++ bug above, we still have to contend
        // with the fact that std::hash<vector<bool>> is often poor quality, hashing
        // directly on the internal words and on no other state.  On these platforms,
        // vector<bool>{1, 1} and vector<bool>{1, 1, 0} hash to the same value.
        //
        // Mixing in the size (as we do in our other vector<> implementations) on top
        // of the library-provided hash implementation avoids this QOI issue.
        template<typename H, typename T, typename Allocator>
        typename std::enable_if<is_hashable<T>::value && std::is_same<T, bool>::value,
                H>::type
        turbo_hash_value(H hash_state, const std::vector<T, Allocator> &vector) {
            return H::combine(std::move(hash_state),
                              std::hash<std::vector<T, Allocator>>{}(vector),
                              vector.size());
        }

#endif

        // -----------------------------------------------------------------------------
        // turbo_hash_value for Ordered Associative Containers
        // -----------------------------------------------------------------------------

        // turbo_hash_value for hashing std::map
        template<typename H, typename Key, typename T, typename Compare,
                typename Allocator>
        typename std::enable_if<is_hashable<Key>::value && is_hashable<T>::value,
                H>::type
        turbo_hash_value(H hash_state, const std::map<Key, T, Compare, Allocator> &map) {
            for (const auto &t: map) {
                hash_state = H::combine(std::move(hash_state), t);
            }
            return H::combine(std::move(hash_state), map.size());
        }

        // turbo_hash_value for hashing std::multimap
        template<typename H, typename Key, typename T, typename Compare,
                typename Allocator>
        typename std::enable_if<is_hashable<Key>::value && is_hashable<T>::value,
                H>::type
        turbo_hash_value(H hash_state,
                         const std::multimap<Key, T, Compare, Allocator> &map) {
            for (const auto &t: map) {
                hash_state = H::combine(std::move(hash_state), t);
            }
            return H::combine(std::move(hash_state), map.size());
        }

        // turbo_hash_value for hashing std::set
        template<typename H, typename Key, typename Compare, typename Allocator>
        typename std::enable_if<is_hashable<Key>::value, H>::type turbo_hash_value(
                H hash_state, const std::set<Key, Compare, Allocator> &set) {
            for (const auto &t: set) {
                hash_state = H::combine(std::move(hash_state), t);
            }
            return H::combine(std::move(hash_state), set.size());
        }

        // turbo_hash_value for hashing std::multiset
        template<typename H, typename Key, typename Compare, typename Allocator>
        typename std::enable_if<is_hashable<Key>::value, H>::type turbo_hash_value(
                H hash_state, const std::multiset<Key, Compare, Allocator> &set) {
            for (const auto &t: set) {
                hash_state = H::combine(std::move(hash_state), t);
            }
            return H::combine(std::move(hash_state), set.size());
        }

        // -----------------------------------------------------------------------------
        // turbo_hash_value for Unordered Associative Containers
        // -----------------------------------------------------------------------------

        // turbo_hash_value for hashing std::unordered_set
        template<typename H, typename Key, typename Hash, typename KeyEqual,
                typename Alloc>
        typename std::enable_if<is_hashable<Key>::value, H>::type turbo_hash_value(
                H hash_state, const std::unordered_set<Key, Hash, KeyEqual, Alloc> &s) {
            return H::combine(
                    H::combine_unordered(std::move(hash_state), s.begin(), s.end()),
                    s.size());
        }

        // turbo_hash_value for hashing std::unordered_multiset
        template<typename H, typename Key, typename Hash, typename KeyEqual,
                typename Alloc>
        typename std::enable_if<is_hashable<Key>::value, H>::type turbo_hash_value(
                H hash_state,
                const std::unordered_multiset<Key, Hash, KeyEqual, Alloc> &s) {
            return H::combine(
                    H::combine_unordered(std::move(hash_state), s.begin(), s.end()),
                    s.size());
        }

        // turbo_hash_value for hashing std::unordered_set
        template<typename H, typename Key, typename T, typename Hash,
                typename KeyEqual, typename Alloc>
        typename std::enable_if<is_hashable<Key>::value && is_hashable<T>::value,
                H>::type
        turbo_hash_value(H hash_state,
                         const std::unordered_map<Key, T, Hash, KeyEqual, Alloc> &s) {
            return H::combine(
                    H::combine_unordered(std::move(hash_state), s.begin(), s.end()),
                    s.size());
        }

        // turbo_hash_value for hashing std::unordered_multiset
        template<typename H, typename Key, typename T, typename Hash,
                typename KeyEqual, typename Alloc>
        typename std::enable_if<is_hashable<Key>::value && is_hashable<T>::value,
                H>::type
        turbo_hash_value(H hash_state,
                         const std::unordered_multimap<Key, T, Hash, KeyEqual, Alloc> &s) {
            return H::combine(
                    H::combine_unordered(std::move(hash_state), s.begin(), s.end()),
                    s.size());
        }

        // -----------------------------------------------------------------------------
        // turbo_hash_value for Wrapper Types
        // -----------------------------------------------------------------------------

        // turbo_hash_value for hashing std::reference_wrapper
        template<typename H, typename T>
        typename std::enable_if<is_hashable<T>::value, H>::type turbo_hash_value(
                H hash_state, std::reference_wrapper<T> opt) {
            return H::combine(std::move(hash_state), opt.get());
        }

        // turbo_hash_value for hashing std::optional
        template<typename H, typename T>
        typename std::enable_if<is_hashable<T>::value, H>::type turbo_hash_value(
                H hash_state, const std::optional<T> &opt) {
            if (opt) hash_state = H::combine(std::move(hash_state), *opt);
            return H::combine(std::move(hash_state), opt.has_value());
        }

        // VariantVisitor
        template<typename H>
        struct VariantVisitor {
            H &&hash_state;

            template<typename T>
            H operator()(const T &t) const {
                return H::combine(std::move(hash_state), t);
            }
        };

        // turbo_hash_value for hashing std::variant
        template<typename H, typename... T>
        typename std::enable_if<conjunction<is_hashable<T>...>::value, H>::type
        turbo_hash_value(H hash_state, const std::variant<T...> &v) {
            if (!v.valueless_by_exception()) {
                hash_state = std::visit(VariantVisitor<H>{std::move(hash_state)}, v);
            }
            return H::combine(std::move(hash_state), v.index());
        }

// -----------------------------------------------------------------------------
// turbo_hash_value for Other Types
// -----------------------------------------------------------------------------

// turbo_hash_value for hashing std::bitset is not defined on Little Endian
// platforms, for the same reason as for vector<bool> (see std::vector above):
// It does not expose the raw bytes, and a fallback to std::hash<> is most
// likely faster.

#if defined(TURBO_IS_BIG_ENDIAN) && \
    (defined(__GLIBCXX__) || defined(__GLIBCPP__))
        // turbo_hash_value for hashing std::bitset
        //
        // std::hash in libstdc++ does not work correctly with std::bitset on Big Endian
        // platforms therefore we need to implement a custom turbo_hash_value for it. More
        // details on the bug: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102531
        template <typename H, size_t N>
        H turbo_hash_value(H hash_state, const std::bitset<N>& set) {
          typename H::TurboInternalPiecewiseCombiner combiner;
          for (size_t i = 0; i < N; i++) {
            unsigned char c = static_cast<unsigned char>(set[i]);
            hash_state = combiner.add_buffer(std::move(hash_state), &c, sizeof(c));
          }
          return H::combine(combiner.finalize(std::move(hash_state)), N);
        }
#endif

        // -----------------------------------------------------------------------------

        // hash_range_or_bytes()
        //
        // Mixes all values in the range [data, data+size) into the hash state.
        // This overload accepts only uniquely-represented types, and hashes them by
        // hashing the entire range of bytes.
        template<typename H, typename T>
        typename std::enable_if<is_uniquely_represented<T>::value, H>::type
        hash_range_or_bytes(H hash_state, const T *data, size_t size) {
            const auto *bytes = reinterpret_cast<const unsigned char *>(data);
            return H::combine_contiguous(std::move(hash_state), bytes, sizeof(T) * size);
        }

        // hash_range_or_bytes()
        template<typename H, typename T>
        typename std::enable_if<!is_uniquely_represented<T>::value, H>::type
        hash_range_or_bytes(H hash_state, const T *data, size_t size) {
            for (const auto end = data + size; data < end; ++data) {
                hash_state = H::combine(std::move(hash_state), *data);
            }
            return hash_state;
        }

#if defined(TURBO_INTERNAL_LEGACY_HASH_NAMESPACE) && \
    TURBO_META_INTERNAL_STD_HASH_SFINAE_FRIENDLY_
#define TURBO_HASH_INTERNAL_SUPPORT_LEGACY_HASH_ 1
#else
#define TURBO_HASH_INTERNAL_SUPPORT_LEGACY_HASH_ 0
#endif

        // HashSelect
        //
        // Type trait to select the appropriate hash implementation to use.
        // HashSelect::type<T> will give the proper hash implementation, to be invoked
        // as:
        //   HashSelect::type<T>::Invoke(state, value)
        // Also, HashSelect::type<T>::value is a boolean equal to `true` if there is a
        // valid `Invoke` function. Types that are not hashable will have a ::value of
        // `false`.
        struct HashSelect {
        private:
            struct State : HashStateBase<State> {
                static State combine_contiguous(State hash_state, const unsigned char *,
                                                size_t);

                using State::HashStateBase::combine_contiguous;
            };

            struct UniquelyRepresentedProbe {
                template<typename H, typename T>
                static auto Invoke(H state, const T &value)
                -> turbo::enable_if_t<is_uniquely_represented<T>::value, H> {
                    return hash_internal::hash_bytes(std::move(state), value);
                }
            };

            struct HashValueProbe {
                template<typename H, typename T>
                static auto Invoke(H state, const T &value) -> turbo::enable_if_t<
                        std::is_same<H,
                                decltype(turbo_hash_value(std::move(state), value))>::value,
                        H> {
                    return turbo_hash_value(std::move(state), value);
                }
            };

            struct LegacyHashProbe {
#if TURBO_HASH_INTERNAL_SUPPORT_LEGACY_HASH_
                template <typename H, typename T>
                static auto Invoke(H state, const T& value) -> turbo::enable_if_t<
                    std::is_convertible<
                        decltype(TURBO_INTERNAL_LEGACY_HASH_NAMESPACE::hash<T>()(value)),
                        size_t>::value,
                    H> {
                  return hash_internal::hash_bytes(
                      std::move(state),
                      TURBO_INTERNAL_LEGACY_HASH_NAMESPACE::hash<T>{}(value));
                }
#endif  // TURBO_HASH_INTERNAL_SUPPORT_LEGACY_HASH_
            };

            struct StdHashProbe {
                template<typename H, typename T>
                static auto Invoke(H state, const T &value)
                -> turbo::enable_if_t<type_traits_internal::IsHashable<T>::value, H> {
                    return hash_internal::hash_bytes(std::move(state), std::hash<T>{}(value));
                }
            };

            template<typename Hash, typename T>
            struct Probe : Hash {
            private:
                template<typename H, typename = decltype(H::Invoke(
                        std::declval<State>(), std::declval<const T &>()))>
                static std::true_type Test(int);

                template<typename U>
                static std::false_type Test(char);

            public:
                static constexpr bool value = decltype(Test<Hash>(0))::value;
            };

        public:
            // Probe each implementation in order.
            // disjunction provides short circuiting wrt instantiation.
            template<typename T>
            using Apply = turbo::disjunction<         //
                    Probe<UniquelyRepresentedProbe, T>,  //
                    Probe<HashValueProbe, T>,            //
                    Probe<LegacyHashProbe, T>,           //
                    Probe<StdHashProbe, T>,              //
                    std::false_type>;
        };

        template<typename T>
        struct is_hashable
                : std::integral_constant<bool, HashSelect::template Apply<T>::value> {
        };

// MixingHashState
        class TURBO_DLL MixingHashState : public HashStateBase<MixingHashState> {
            // turbo::uint128 is not an alias or a thin wrapper around the intrinsic.
            // We use the intrinsic when available to improve performance.
#ifdef TURBO_HAVE_INTRINSIC_INT128
            using uint128 = __uint128_t;
#else   // TURBO_HAVE_INTRINSIC_INT128
            using uint128 = turbo::uint128;
#endif  // TURBO_HAVE_INTRINSIC_INT128

            static constexpr uint64_t kMul =
                    sizeof(size_t) == 4 ? uint64_t{0xcc9e2d51}
                                        : uint64_t{0x9ddfea08eb382d69};

            template<typename T>
            using IntegralFastPath =
                    conjunction<std::is_integral<T>, is_uniquely_represented<T>>;

        public:
            // Move only
            MixingHashState(MixingHashState &&) = default;

            MixingHashState &operator=(MixingHashState &&) = default;

            // MixingHashState::combine_contiguous()
            //
            // Fundamental base case for hash recursion: mixes the given range of bytes
            // into the hash state.
            static MixingHashState combine_contiguous(MixingHashState hash_state,
                                                      const unsigned char *first,
                                                      size_t size) {
                return MixingHashState(
                        CombineContiguousImpl(hash_state.state_, first, size,
                                              std::integral_constant<int, sizeof(size_t)>{}));
            }

            using MixingHashState::HashStateBase::combine_contiguous;

            // MixingHashState::hash()
            //
            // For performance reasons in non-opt mode, we specialize this for
            // integral types.
            // Otherwise we would be instantiating and calling dozens of functions for
            // something that is just one multiplication and a couple xor's.
            // The result should be the same as running the whole algorithm, but faster.
            template<typename T, turbo::enable_if_t<IntegralFastPath<T>::value, int> = 0>
            static size_t hash(T value) {
                return static_cast<size_t>(
                        Mix(Seed(), static_cast<std::make_unsigned_t<T>>(value)));
            }

            // Overload of MixingHashState::hash()
            template<typename T, turbo::enable_if_t<!IntegralFastPath<T>::value, int> = 0>
            static size_t hash(const T &value) {
                return static_cast<size_t>(combine(MixingHashState{}, value).state_);
            }

        private:
            // Invoked only once for a given argument; that plus the fact that this is
            // move-only ensures that there is only one non-moved-from object.
            MixingHashState() : state_(Seed()) {}

            friend class MixingHashState::HashStateBase;

            template<typename CombinerT>
            static MixingHashState RunCombineUnordered(MixingHashState state,
                                                       CombinerT combiner) {
                uint64_t unordered_state = 0;
                combiner(MixingHashState{}, [&](MixingHashState &inner_state) {
                    // Add the hash state of the element to the running total, but mix the
                    // carry bit back into the low bit.  This in intended to avoid losing
                    // entropy to overflow, especially when unordered_multisets contain
                    // multiple copies of the same value.
                    auto element_state = inner_state.state_;
                    unordered_state += element_state;
                    if (unordered_state < element_state) {
                        ++unordered_state;
                    }
                    inner_state = MixingHashState{};
                });
                return MixingHashState::combine(std::move(state), unordered_state);
            }

            // Allow the HashState type-erasure implementation to invoke
            // RunCombinedUnordered() directly.
            friend class turbo::HashState;

            // Workaround for MSVC bug.
            // We make the type copyable to fix the calling convention, even though we
            // never actually copy it. Keep it private to not affect the public API of the
            // type.
            MixingHashState(const MixingHashState &) = default;

            explicit MixingHashState(uint64_t state) : state_(state) {}

            // Implementation of the base case for combine_contiguous where we actually
            // mix the bytes into the state.
            // Dispatch to different implementations of the combine_contiguous depending
            // on the value of `sizeof(size_t)`.
            static uint64_t CombineContiguousImpl(uint64_t state,
                                                  const unsigned char *first, size_t len,
                                                  std::integral_constant<int, 4>
                                                  /* sizeof_size_t */);

            static uint64_t CombineContiguousImpl(uint64_t state,
                                                  const unsigned char *first, size_t len,
                                                  std::integral_constant<int, 8>
                                                  /* sizeof_size_t */);

            // Slow dispatch path for calls to CombineContiguousImpl with a size argument
            // larger than PiecewiseChunkSize().  Has the same effect as calling
            // CombineContiguousImpl() repeatedly with the chunk stride size.
            static uint64_t CombineLargeContiguousImpl32(uint64_t state,
                                                         const unsigned char *first,
                                                         size_t len);

            static uint64_t CombineLargeContiguousImpl64(uint64_t state,
                                                         const unsigned char *first,
                                                         size_t len);

            // Reads 9 to 16 bytes from p.
            // The least significant 8 bytes are in .first, the rest (zero padded) bytes
            // are in .second.
            static std::pair<uint64_t, uint64_t> Read9To16(const unsigned char *p,
                                                           size_t len) {
                uint64_t low_mem = turbo::base_internal::UnalignedLoad64(p);
                uint64_t high_mem = turbo::base_internal::UnalignedLoad64(p + len - 8);
#ifdef TURBO_IS_LITTLE_ENDIAN
                uint64_t most_significant = high_mem;
                uint64_t least_significant = low_mem;
#else
                uint64_t most_significant = low_mem;
                uint64_t least_significant = high_mem;
#endif
                return {least_significant, most_significant};
            }

            // Reads 4 to 8 bytes from p. Zero pads to fill uint64_t.
            static uint64_t Read4To8(const unsigned char *p, size_t len) {
                uint32_t low_mem = turbo::base_internal::UnalignedLoad32(p);
                uint32_t high_mem = turbo::base_internal::UnalignedLoad32(p + len - 4);
#ifdef TURBO_IS_LITTLE_ENDIAN
                uint32_t most_significant = high_mem;
                uint32_t least_significant = low_mem;
#else
                uint32_t most_significant = low_mem;
                uint32_t least_significant = high_mem;
#endif
                return (static_cast<uint64_t>(most_significant) << (len - 4) * 8) |
                       least_significant;
            }

            // Reads 1 to 3 bytes from p. Zero pads to fill uint32_t.
            static uint32_t Read1To3(const unsigned char *p, size_t len) {
                // The trick used by this implementation is to avoid branches if possible.
                unsigned char mem0 = p[0];
                unsigned char mem1 = p[len / 2];
                unsigned char mem2 = p[len - 1];
#ifdef TURBO_IS_LITTLE_ENDIAN
                unsigned char significant2 = mem2;
                unsigned char significant1 = mem1;
                unsigned char significant0 = mem0;
#else
                unsigned char significant2 = mem0;
                unsigned char significant1 = len == 2 ? mem0 : mem1;
                unsigned char significant0 = mem2;
#endif
                return static_cast<uint32_t>(significant0 |                     //
                                             (significant1 << (len / 2 * 8)) |  //
                                             (significant2 << ((len - 1) * 8)));
            }

            TURBO_ATTRIBUTE_ALWAYS_INLINE static uint64_t Mix(uint64_t state, uint64_t v) {
                // Though the 128-bit product on AArch64 needs two instructions, it is
                // still a good balance between speed and hash quality.
                using MultType =
                        turbo::conditional_t<sizeof(size_t) == 4, uint64_t, uint128>;
                // We do the addition in 64-bit space to make sure the 128-bit
                // multiplication is fast. If we were to do it as MultType the compiler has
                // to assume that the high word is non-zero and needs to perform 2
                // multiplications instead of one.
                MultType m = state + v;
                m *= kMul;
                return static_cast<uint64_t>(m ^ (m >> (sizeof(m) * 8 / 2)));
            }

            // An extern to avoid bloat on a direct call to LowLevelHash() with fixed
            // values for both the seed and salt parameters.
            static uint64_t LowLevelHashImpl(const unsigned char *data, size_t len);

            TURBO_ATTRIBUTE_ALWAYS_INLINE static uint64_t Hash64(const unsigned char *data,
                                                                 size_t len) {
#ifdef TURBO_HAVE_INTRINSIC_INT128
                return LowLevelHashImpl(data, len);
#else
                return hash_internal::CityHash64(reinterpret_cast<const char*>(data), len);
#endif
            }

            // Seed()
            //
            // A non-deterministic seed.
            //
            // The current purpose of this seed is to generate non-deterministic results
            // and prevent having users depend on the particular hash values.
            // It is not meant as a security feature right now, but it leaves the door
            // open to upgrade it to a true per-process random seed. A true random seed
            // costs more and we don't need to pay for that right now.
            //
            // On platforms with ASLR, we take advantage of it to make a per-process
            // random value.
            // See https://en.wikipedia.org/wiki/Address_space_layout_randomization
            //
            // On other platforms this is still going to be non-deterministic but most
            // probably per-build and not per-process.
            TURBO_ATTRIBUTE_ALWAYS_INLINE static uint64_t Seed() {
#if (!defined(__clang__) || __clang_major__ > 11) && \
    (!defined(__apple_build_version__) || \
     __apple_build_version__ >= 19558921)  // Xcode 12
                return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(&kSeed));
#else
                // Workaround the absence of
                // https://github.com/llvm/llvm-project/commit/bc15bf66dcca76cc06fe71fca35b74dc4d521021.
                return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(kSeed));
#endif
            }

            static const void *const kSeed;

            uint64_t state_;
        };

// MixingHashState::CombineContiguousImpl()
        inline uint64_t MixingHashState::CombineContiguousImpl(
                uint64_t state, const unsigned char *first, size_t len,
                std::integral_constant<int, 4> /* sizeof_size_t */) {
            // For large values we use CityHash, for small ones we just use a
            // multiplicative hash.
            uint64_t v;
            if (len > 8) {
                if (TURBO_UNLIKELY(len > PiecewiseChunkSize())) {
                    return CombineLargeContiguousImpl32(state, first, len);
                }
                v = hash_internal::CityHash32(reinterpret_cast<const char *>(first), len);
            } else if (len >= 4) {
                v = Read4To8(first, len);
            } else if (len > 0) {
                v = Read1To3(first, len);
            } else {
                // Empty ranges have no effect.
                return state;
            }
            return Mix(state, v);
        }

        // Overload of MixingHashState::CombineContiguousImpl()
        inline uint64_t MixingHashState::CombineContiguousImpl(
                uint64_t state, const unsigned char *first, size_t len,
                std::integral_constant<int, 8> /* sizeof_size_t */) {
            // For large values we use LowLevelHash or CityHash depending on the platform,
            // for small ones we just use a multiplicative hash.
            uint64_t v;
            if (len > 16) {
                if (TURBO_UNLIKELY(len > PiecewiseChunkSize())) {
                    return CombineLargeContiguousImpl64(state, first, len);
                }
                v = Hash64(first, len);
            } else if (len > 8) {
                // This hash function was constructed by the ML-driven algorithm discovery
                // using reinforcement learning. We fed the agent lots of inputs from
                // microbenchmarks, SMHasher, low hamming distance from generated inputs and
                // picked up the one that was good on micro and macrobenchmarks.
                auto p = Read9To16(first, len);
                uint64_t lo = p.first;
                uint64_t hi = p.second;
                // Rotation by 53 was found to be most often useful when discovering these
                // hashing algorithms with ML techniques.
                lo = turbo::rotr(lo, 53);
                state += kMul;
                lo += state;
                state ^= hi;
                uint128 m = state;
                m *= lo;
                return static_cast<uint64_t>(m ^ (m >> 64));
            } else if (len >= 4) {
                v = Read4To8(first, len);
            } else if (len > 0) {
                v = Read1To3(first, len);
            } else {
                // Empty ranges have no effect.
                return state;
            }
            return Mix(state, v);
        }

        struct AggregateBarrier {
        };

        // HashImpl

        // Add a private base class to make sure this type is not an aggregate.
        // Aggregates can be aggregate initialized even if the default constructor is
        // deleted.
        struct PoisonedHash : private AggregateBarrier {
            PoisonedHash() = delete;

            PoisonedHash(const PoisonedHash &) = delete;

            PoisonedHash &operator=(const PoisonedHash &) = delete;
        };

        template<typename T>
        struct HashImpl {
            size_t operator()(const T &value) const {
                return MixingHashState::hash(value);
            }
        };

        template<typename T>
        struct Hash
                : turbo::conditional_t<is_hashable<T>::value, HashImpl<T>, PoisonedHash> {
        };

        template<typename H>
        template<typename T, typename... Ts>
        H HashStateBase<H>::combine(H state, const T &value, const Ts &... values) {
            return H::combine(hash_internal::HashSelect::template Apply<T>::Invoke(
                                      std::move(state), value),
                              values...);
        }

        // HashStateBase::combine_contiguous()
        template<typename H>
        template<typename T>
        H HashStateBase<H>::combine_contiguous(H state, const T *data, size_t size) {
            return hash_internal::hash_range_or_bytes(std::move(state), data, size);
        }

        // HashStateBase::combine_unordered()
        template<typename H>
        template<typename I>
        H HashStateBase<H>::combine_unordered(H state, I begin, I end) {
            return H::RunCombineUnordered(std::move(state),
                                          CombineUnorderedCallback < I > {begin, end});
        }

        // HashStateBase::PiecewiseCombiner::add_buffer()
        template<typename H>
        H PiecewiseCombiner::add_buffer(H state, const unsigned char *data,
                                        size_t size) {
            if (position_ + size < PiecewiseChunkSize()) {
                // This partial chunk does not fill our existing buffer
                memcpy(buf_ + position_, data, size);
                position_ += size;
                return state;
            }

            // If the buffer is partially filled we need to complete the buffer
            // and hash it.
            if (position_ != 0) {
                const size_t bytes_needed = PiecewiseChunkSize() - position_;
                memcpy(buf_ + position_, data, bytes_needed);
                state = H::combine_contiguous(std::move(state), buf_, PiecewiseChunkSize());
                data += bytes_needed;
                size -= bytes_needed;
            }

            // Hash whatever chunks we can without copying
            while (size >= PiecewiseChunkSize()) {
                state = H::combine_contiguous(std::move(state), data, PiecewiseChunkSize());
                data += PiecewiseChunkSize();
                size -= PiecewiseChunkSize();
            }
            // Fill the buffer with the remainder
            memcpy(buf_, data, size);
            position_ = size;
            return state;
        }

        // HashStateBase::PiecewiseCombiner::finalize()
        template<typename H>
        H PiecewiseCombiner::finalize(H state) {
            // Hash the remainder left in the buffer, which may be empty
            return H::combine_contiguous(std::move(state), buf_, position_);
        }

    }  // namespace hash_internal
}  // namespace turbo
