// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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


#ifndef TURBO_HASH_INTERNAL_MIXING_HASH_STATE_H_
#define TURBO_HASH_INTERNAL_MIXING_HASH_STATE_H_

#include "turbo/base/bits.h"
#include "turbo/platform/port.h"
#include "turbo/hash/internal/hash_state_base.h"
#include "turbo/hash/hash_engine.h"
#include "turbo/base/endian.h"
#include "turbo/hash/fwd.h"

namespace turbo {
    class HashState;
} // namespace turbo

namespace turbo::hash_internal {

    // MixingHashState
    template<typename Tag>
    class TURBO_DLL MixingHashState : public HashStateBase<MixingHashState<Tag>> {
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
                std::conjunction<std::is_integral<T>, is_uniquely_represented<T>>;

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
        template<typename T, std::enable_if_t<IntegralFastPath<T>::value, int> = 0>
        static size_t hash(T value) {
            return static_cast<size_t>(Mix(Seed(), static_cast<uint64_t>(value)));
        }

        // Overload of MixingHashState::hash()
        template<typename T, std::enable_if_t<!IntegralFastPath<T>::value, int> = 0>
        static size_t hash(const T &value) {
            return static_cast<size_t>( HashStateBase<MixingHashState<Tag>>::combine(MixingHashState{}, value).state_);
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
            return  HashStateBase<MixingHashState<Tag>>::combine(std::move(state), unordered_state);
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
#if TURBO_IS_LITTLE_ENDIAN
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
#if TURBO_IS_LITTLE_ENDIAN
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
            unsigned char mem0 = p[0];
            unsigned char mem1 = p[len / 2];
            unsigned char mem2 = p[len - 1];
#if TURBO_IS_LITTLE_ENDIAN
            unsigned char significant2 = mem2;
            unsigned char significant1 = mem1;
            unsigned char significant0 = mem0;
#else
            unsigned char significant2 = mem0;
                unsigned char significant1 = mem1;
                unsigned char significant0 = mem2;
#endif
            return static_cast<uint32_t>(significant0 |                     //
                                         (significant1 << (len / 2 * 8)) |  //
                                         (significant2 << ((len - 1) * 8)));
        }

        TURBO_FORCE_INLINE static uint64_t Mix(uint64_t state, uint64_t v) {
            // Though the 128-bit product on AArch64 needs two instructions, it is
            // still a good balance between speed and hash quality.
            using MultType =
                    std::conditional_t<sizeof(size_t) == 4, uint64_t, uint128>;
            // We do the addition in 64-bit space to make sure the 128-bit
            // multiplication is fast. If we were to do it as MultType the compiler has
            // to assume that the high word is non-zero and needs to perform 2
            // multiplications instead of one.
            MultType m = state + v;
            m *= kMul;
            return static_cast<uint64_t>(m ^ (m >> (sizeof(m) * 8 / 2)));
        }

        TURBO_FORCE_INLINE static uint64_t Hash64(const unsigned char *data,
                                                  size_t len) {
            return hasher_engine<Tag>::hash64(reinterpret_cast<const char *>(data), len);
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
        TURBO_FORCE_INLINE static uint64_t Seed() {
#if (!defined(__clang__) || __clang_major__ > 11) && \
    !defined(__apple_build_version__)
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
    template<typename Tag>
    inline uint64_t MixingHashState<Tag>::CombineContiguousImpl(
            uint64_t state, const unsigned char *first, size_t len,
            std::integral_constant<int, 4> /* sizeof_size_t */) {
        // For large values we use CityHash, for small ones we just use a
        // multiplicative hash.
        uint64_t v;
        if (len > 8) {
            if (TURBO_UNLIKELY(len > PiecewiseChunkSize())) {
                return CombineLargeContiguousImpl32(state, first, len);
            }
            v = hasher_engine<Tag>::hash32(reinterpret_cast<const char *>(first), len);
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
    template<typename Tag>
    inline uint64_t MixingHashState<Tag>::CombineContiguousImpl(
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

    extern template class MixingHashState<city_hash_tag>;
    extern template class MixingHashState<bytes_hash_tag>;
    extern template class MixingHashState<m3_hash_tag>;
    extern template class MixingHashState<xx_hash_tag>;
}  // namespace turbo::hash_internal

#endif  // TURBO_HASH_INTERNAL_MIXING_HASH_STATE_H_
