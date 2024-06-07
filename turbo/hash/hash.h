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
// This header file defines the Turbo `hash` library and the Turbo hashing
// framework. This framework consists of the following:
//
//   * The `turbo::Hash` functor, which is used to invoke the hasher within the
//     Turbo hashing framework. `turbo::Hash<T>` supports most basic types and
//     a number of Turbo types out of the box.
//   * `turbo_hash_value`, an extension point that allows you to extend types to
//     support Turbo hashing without requiring you to define a hashing
//     algorithm.
//   * `HashState`, a type-erased class which implements the manipulation of the
//     hash state (H) itself; contains member functions `combine()`,
//     `combine_contiguous()`, and `combine_unordered()`; and which you can use
//     to contribute to an existing hash state when hashing your types.
//
// Unlike `std::hash` or other hashing frameworks, the Turbo hashing framework
// provides most of its utility by abstracting away the hash algorithm (and its
// implementation) entirely. Instead, a type invokes the Turbo hashing
// framework by simply combining its state with the state of known, hashable
// types. Hashing of that combined state is separately done by `turbo::Hash`.
//
// One should assume that a hash algorithm is chosen randomly at the start of
// each process.  E.g., `turbo::Hash<int>{}(9)` in one process and
// `turbo::Hash<int>{}(9)` in another process are likely to differ.
//
// `turbo::Hash` may also produce different values from different dynamically
// loaded libraries. For this reason, `turbo::Hash` values must never cross
// boundaries in dynamically loaded libraries (including when used in types like
// hash containers.)
//
// `turbo::Hash` is intended to strongly mix input bits with a target of passing
// an [Avalanche Test](https://en.wikipedia.org/wiki/Avalanche_effect).
//
// Example:
//
//   // Suppose we have a class `Circle` for which we want to add hashing:
//   class Circle {
//    public:
//     ...
//    private:
//     std::pair<int, int> center_;
//     int radius_;
//   };
//
//   // To add hashing support to `Circle`, we simply need to add a free
//   // (non-member) function `turbo_hash_value()`, and return the combined hash
//   // state of the existing hash state and the class state. You can add such a
//   // free function using a friend declaration within the body of the class:
//   class Circle {
//    public:
//     ...
//     template <typename H>
//     friend H turbo_hash_value(H h, const Circle& c) {
//       return H::combine(std::move(h), c.center_, c.radius_);
//     }
//     ...
//   };
//
// For more information, see Adding Type Support to `turbo::Hash` below.
//
#pragma once

#include <tuple>
#include <utility>
#include <array>
#include <turbo/functional/function_ref.h>
#include <turbo/hash/internal/hash.h>
#include <turbo/container/span.h>

namespace turbo {

    // -----------------------------------------------------------------------------
    // `turbo::Hash`
    // -----------------------------------------------------------------------------
    //
    // `turbo::Hash<T>` is a convenient general-purpose hash functor for any type `T`
    // satisfying any of the following conditions (in order):
    //
    //  * T is an arithmetic or pointer type
    //  * T defines an overload for `turbo_hash_value(H, const T&)` for an arbitrary
    //    hash state `H`.
    //  - T defines a specialization of `std::hash<T>`
    //
    // `turbo::Hash` intrinsically supports the following types:
    //
    //   * All integral types (including bool)
    //   * All enum types
    //   * All floating-point types (although hashing them is discouraged)
    //   * All pointer types, including nullptr_t
    //   * std::pair<T1, T2>, if T1 and T2 are hashable
    //   * std::tuple<Ts...>, if all the Ts... are hashable
    //   * std::unique_ptr and std::shared_ptr
    //   * All string-like types including:
    //     * turbo::Cord
    //     * std::string (as well as any instance of std::basic_string that
    //       uses one of {char, wchar_t, char16_t, char32_t} and its associated
    //       std::char_traits)
    //     * std::string_view (as well as any instance of std::basic_string_view
    //       that uses one of {char, wchar_t, char16_t, char32_t} and its associated
    //       std::char_traits)
    //  * All the standard sequence containers (provided the elements are hashable)
    //  * All the standard associative containers (provided the elements are
    //    hashable)
    //  * turbo types such as the following:
    //    * turbo::string_view
    //    * turbo::uint128
    //    * turbo::Time, turbo::Duration, and turbo::TimeZone
    //  * turbo containers (provided the elements are hashable) such as the
    //    following:
    //    * turbo::flat_hash_set, turbo::node_hash_set, turbo::btree_set
    //    * turbo::flat_hash_map, turbo::node_hash_map, turbo::btree_map
    //    * turbo::btree_multiset, turbo::btree_multimap
    //    * turbo::InlinedVector
    //    * turbo::FixedArray
    //
    // When turbo::Hash is used to hash an unordered container with a custom hash
    // functor, the elements are hashed using default turbo::Hash semantics, not
    // the custom hash functor.  This is consistent with the behavior of
    // operator==() on unordered containers, which compares elements pairwise with
    // operator==() rather than the custom equality functor.  It is usually a
    // mistake to use either operator==() or turbo::Hash on unordered collections
    // that use functors incompatible with operator==() equality.
    //
    // Note: the list above is not meant to be exhaustive. Additional type support
    // may be added, in which case the above list will be updated.
    //
    // -----------------------------------------------------------------------------
    // turbo::Hash Invocation Evaluation
    // -----------------------------------------------------------------------------
    //
    // When invoked, `turbo::Hash<T>` searches for supplied hash functions in the
    // following order:
    //
    //   * Natively supported types out of the box (see above)
    //   * Types for which an `turbo_hash_value()` overload is provided (such as
    //     user-defined types). See "Adding Type Support to `turbo::Hash`" below.
    //   * Types which define a `std::hash<T>` specialization
    //
    // The fallback to legacy hash functions exists mainly for backwards
    // compatibility. If you have a choice, prefer defining an `turbo_hash_value`
    // overload instead of specializing any legacy hash functors.
    //
    // -----------------------------------------------------------------------------
    // The Hash State Concept, and using `HashState` for Type Erasure
    // -----------------------------------------------------------------------------
    //
    // The `turbo::Hash` framework relies on the Concept of a "hash state." Such a
    // hash state is used in several places:
    //
    // * Within existing implementations of `turbo::Hash<T>` to store the hashed
    //   state of an object. Note that it is up to the implementation how it stores
    //   such state. A hash table, for example, may mix the state to produce an
    //   integer value; a testing framework may simply hold a vector of that state.
    // * Within implementations of `turbo_hash_value()` used to extend user-defined
    //   types. (See "Adding Type Support to turbo::Hash" below.)
    // * Inside a `HashState`, providing type erasure for the concept of a hash
    //   state, which you can use to extend the `turbo::Hash` framework for types
    //   that are otherwise difficult to extend using `turbo_hash_value()`. (See the
    //   `HashState` class below.)
    //
    // The "hash state" concept contains three member functions for mixing hash
    // state:
    //
    // * `H::combine(state, values...)`
    //
    //   Combines an arbitrary number of values into a hash state, returning the
    //   updated state. Note that the existing hash state is move-only and must be
    //   passed by value.
    //
    //   Each of the value types T must be hashable by H.
    //
    //   NOTE:
    //
    //     state = H::combine(std::move(state), value1, value2, value3);
    //
    //   must be guaranteed to produce the same hash expansion as
    //
    //     state = H::combine(std::move(state), value1);
    //     state = H::combine(std::move(state), value2);
    //     state = H::combine(std::move(state), value3);
    //
    // * `H::combine_contiguous(state, data, size)`
    //
    //    Combines a contiguous array of `size` elements into a hash state,
    //    returning the updated state. Note that the existing hash state is
    //    move-only and must be passed by value.
    //
    //    NOTE:
    //
    //      state = H::combine_contiguous(std::move(state), data, size);
    //
    //    need NOT be guaranteed to produce the same hash expansion as a loop
    //    (it may perform internal optimizations). If you need this guarantee, use a
    //    loop instead.
    //
    // * `H::combine_unordered(state, begin, end)`
    //
    //    Combines a set of elements denoted by an iterator pair into a hash
    //    state, returning the updated state.  Note that the existing hash
    //    state is move-only and must be passed by value.
    //
    //    Unlike the other two methods, the hashing is order-independent.
    //    This can be used to hash unordered collections.
    //
    // -----------------------------------------------------------------------------
    // Adding Type Support to `turbo::Hash`
    // -----------------------------------------------------------------------------
    //
    // To add support for your user-defined type, add a proper `turbo_hash_value()`
    // overload as a free (non-member) function. The overload will take an
    // existing hash state and should combine that state with state from the type.
    //
    // Example:
    //
    //   template <typename H>
    //   H turbo_hash_value(H state, const MyType& v) {
    //     return H::combine(std::move(state), v.field1, ..., v.fieldN);
    //   }
    //
    // where `(field1, ..., fieldN)` are the members you would use on your
    // `operator==` to define equality.
    //
    // Notice that `turbo_hash_value` is not a class member, but an ordinary function.
    // An `turbo_hash_value` overload for a type should only be declared in the same
    // file and namespace as said type. The proper `turbo_hash_value` implementation
    // for a given type will be discovered via ADL.
    //
    // Note: unlike `std::hash', `turbo::Hash` should never be specialized. It must
    // only be extended by adding `turbo_hash_value()` overloads.
    //
    template<typename T>
    using Hash = turbo::hash_internal::Hash<T>;

    // hash_of
    //
    // turbo::hash_of() is a helper that generates a hash from the values of its
    // arguments.  It dispatches to turbo::Hash directly, as follows:
    //  * hash_of(t) == turbo::Hash<T>{}(t)
    //  * hash_of(a, b, c) == hash_of(std::make_tuple(a, b, c))
    //
    // hash_of(a1, a2, ...) == hash_of(b1, b2, ...) is guaranteed when
    //  * The argument lists have pairwise identical C++ types
    //  * a1 == b1 && a2 == b2 && ...
    //
    // The requirement that the arguments match in both type and value is critical.
    // It means that `a == b` does not necessarily imply `hash_of(a) == hash_of(b)` if
    // `a` and `b` have different types. For example, `hash_of(2) != hash_of(2.0)`.
    template<int &... ExplicitArgumentBarrier, typename... Types>
    size_t hash_of(const Types &... values) {
        auto tuple = std::tie(values...);
        return turbo::Hash<decltype(tuple)>{}(tuple);
    }

    // HashState
    //
    // A type erased version of the hash state concept, for use in user-defined
    // `turbo_hash_value` implementations that can't use templates (such as PImpl
    // classes, virtual functions, etc.). The type erasure adds overhead so it
    // should be avoided unless necessary.
    //
    // Note: This wrapper will only erase calls to
    //     combine_contiguous(H, const unsigned char*, size_t)
    //     RunCombineUnordered(H, CombinerF)
    //
    // All other calls will be handled internally and will not invoke overloads
    // provided by the wrapped class.
    //
    // Users of this class should still define a template `turbo_hash_value` function,
    // but can use `turbo::HashState::Create(&state)` to erase the type of the hash
    // state and dispatch to their private hashing logic.
    //
    // This state can be used like any other hash state. In particular, you can call
    // `HashState::combine()` and `HashState::combine_contiguous()` on it.
    //
    // Example:
    //
    //   class Interface {
    //    public:
    //     template <typename H>
    //     friend H turbo_hash_value(H state, const Interface& value) {
    //       state = H::combine(std::move(state), std::type_index(typeid(*this)));
    //       value.HashValue(turbo::HashState::Create(&state));
    //       return state;
    //     }
    //    private:
    //     virtual void HashValue(turbo::HashState state) const = 0;
    //   };
    //
    //   class Impl : Interface {
    //    private:
    //     void HashValue(turbo::HashState state) const override {
    //       turbo::HashState::combine(std::move(state), v1_, v2_);
    //     }
    //     int v1_;
    //     std::string v2_;
    //   };
    class HashState : public hash_internal::HashStateBase<HashState> {
    public:
        // HashState::Create()
        //
        // Create a new `HashState` instance that wraps `state`. All calls to
        // `combine()` and `combine_contiguous()` on the new instance will be
        // redirected to the original `state` object. The `state` object must outlive
        // the `HashState` instance.
        template<typename T>
        static HashState Create(T *state) {
            HashState s;
            s.Init(state);
            return s;
        }

        HashState(const HashState &) = delete;

        HashState &operator=(const HashState &) = delete;

        HashState(HashState &&) = default;

        HashState &operator=(HashState &&) = default;

        // HashState::combine()
        //
        // Combines an arbitrary number of values into a hash state, returning the
        // updated state.
        using HashState::HashStateBase::combine;

        // HashState::combine_contiguous()
        //
        // Combines a contiguous array of `size` elements into a hash state, returning
        // the updated state.
        static HashState combine_contiguous(HashState hash_state,
                                            const unsigned char *first, size_t size) {
            hash_state.combine_contiguous_(hash_state.state_, first, size);
            return hash_state;
        }

        using HashState::HashStateBase::combine_contiguous;

    private:
        HashState() = default;

        friend class HashState::HashStateBase;

        template<typename T>
        static void CombineContiguousImpl(void *p, const unsigned char *first,
                                          size_t size) {
            T &state = *static_cast<T *>(p);
            state = T::combine_contiguous(std::move(state), first, size);
        }

        template<typename T>
        void Init(T *state) {
            state_ = state;
            combine_contiguous_ = &CombineContiguousImpl<T>;
            run_combine_unordered_ = &RunCombineUnorderedImpl<T>;
        }

        template<typename HS>
        struct CombineUnorderedInvoker {
            template<typename T, typename ConsumerT>
            void operator()(T inner_state, ConsumerT inner_cb) {
                f(HashState::Create(&inner_state),
                  [&](HashState &inner_erased) { inner_cb(inner_erased.Real<T>()); });
            }

            turbo::FunctionRef<void(HS, turbo::FunctionRef<void(HS &)>)> f;
        };

        template<typename T>
        static HashState RunCombineUnorderedImpl(
                HashState state,
                turbo::FunctionRef<void(HashState, turbo::FunctionRef<void(HashState &)>)>
                f) {
            // Note that this implementation assumes that inner_state and outer_state
            // are the same type.  This isn't true in the SpyHash case, but SpyHash
            // types are move-convertible to each other, so this still works.
            T &real_state = state.Real<T>();
            real_state = T::RunCombineUnordered(
                    std::move(real_state), CombineUnorderedInvoker<HashState>{f});
            return state;
        }

        template<typename CombinerT>
        static HashState RunCombineUnordered(HashState state, CombinerT combiner) {
            auto *run = state.run_combine_unordered_;
            return run(std::move(state), std::ref(combiner));
        }

        // Do not erase an already erased state.
        void Init(HashState *state) {
            state_ = state->state_;
            combine_contiguous_ = state->combine_contiguous_;
            run_combine_unordered_ = state->run_combine_unordered_;
        }

        template<typename T>
        T &Real() {
            return *static_cast<T *>(state_);
        }

        void *state_;

        void (*combine_contiguous_)(void *, const unsigned char *, size_t);

        HashState (*run_combine_unordered_)(
                HashState state,
                turbo::FunctionRef<void(HashState, turbo::FunctionRef<void(HashState &)>)>);
    };

    // using murmur3 hash algorithm
    // hash32 for bytes, string, vector, etc.
    uint32_t hash32(const void *data, size_t size, uint32_t seed = 0);

    // using murmur3 hash algorithm
    template<typename T>
    inline uint32_t hash32(turbo::Span<T> data, uint32_t seed = 0) {
        return hash32(data.data(), data.size() * sizeof(T), seed);
    }

    // using murmur3 hash algorithm
    // hash32 for string_view
    inline uint32_t hash32(turbo::string_view data, uint32_t seed = 0) {
        return hash32(data.data(), data.size(), seed);
    }

    // using murmur3 hash algorithm
    // hash128 for bytes, string, vector, etc.
    // result is a 128-bit hash value, stored in two 64-bit integers
    void hash128(const void *data, size_t size, std::array<uint64_t, 2> *result, uint32_t seed = 0);

    // using murmur3 hash algorithm
    // hash128 for bytes, string, vector, etc.
    // result is a 128-bit hash value, stored in four 32-bit integers
    void hash128(const void *data, size_t size, std::array<uint32_t , 4> *result, uint32_t seed = 0);

    template<typename T>
    inline void hash128(turbo::Span<T> data, std::array<uint64_t, 2> *result, uint32_t seed = 0) {
        hash128(data.data(), data.size() * sizeof(T), result, seed);
    }

    template<typename T>
    inline void hash128(turbo::Span<T> data, std::array<uint32_t , 4> *result, uint32_t seed = 0) {
        hash128(data.data(), data.size() * sizeof(T), result, seed);
    }

    inline void hash128(turbo::string_view data, std::array<uint64_t, 2> *result, uint32_t seed = 0) {
        hash128(data.data(), data.size(), result, seed);
    }

    inline void hash128(turbo::string_view data, std::array<uint32_t , 4> *result, uint32_t seed = 0) {
        hash128(data.data(), data.size(), result, seed);
    }


}  // namespace turbo
