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

#pragma once

#include <atomic>
#include <cstddef>
#include <deque>

#include <turbo/base/config.h>
#include <turbo/crypto/crc32c.h>

namespace turbo::crc_internal {

    // CrcCordState is a copy-on-write class that holds the chunked CRC32C data
    // that allows CrcCord to perform efficient substring operations. CrcCordState
    // is used as a member variable in CrcCord. When a CrcCord is converted to a
    // Cord, the CrcCordState is shallow-copied into the root node of the Cord. If
    // the converted Cord is modified outside of CrcCord, the CrcCordState is
    // discarded from the Cord. If the Cord is converted back to a CrcCord, and the
    // Cord is still carrying the CrcCordState in its root node, the CrcCord can
    // re-use the CrcCordState, making the construction of the CrcCord cheap.
    //
    // CrcCordState does not try to encapsulate the CRC32C state (CrcCord requires
    // knowledge of how CrcCordState represents the CRC32C state). It does
    // encapsulate the copy-on-write nature of the state.
    class CrcCordState {
    public:
        // Constructors.
        CrcCordState();

        CrcCordState(const CrcCordState &);

        CrcCordState(CrcCordState &&);

        // Destructor. Atomically unreferences the data.
        ~CrcCordState();

        // Copy and move operators.
        CrcCordState &operator=(const CrcCordState &);

        CrcCordState &operator=(CrcCordState &&);

        // A (length, crc) pair.
        struct PrefixCrc {
            PrefixCrc() = default;

            PrefixCrc(size_t length_arg, turbo::CRC32C crc_arg)
                    : length(length_arg), crc(crc_arg) {}

            size_t length = 0;

            // TODO(turbo-team): Memory stomping often zeros out memory. If this struct
            // gets overwritten, we could end up with {0, 0}, which is the correct CRC
            // for a string of length 0. Consider storing a scrambled value and
            // unscrambling it before verifying it.
            turbo::CRC32C crc = turbo::CRC32C{0};
        };

        // The representation of the chunked CRC32C data.
        struct Rep {
            // `removed_prefix` is the crc and length of any prefix that has been
            // removed from the Cord (for example, by calling
            // `CrcCord::remove_prefix()`). To get the checksum of any prefix of the
            // cord, this value must be subtracted from `prefix_crc`. See `Checksum()`
            // for an example.
            //
            // CrcCordState is said to be "normalized" if removed_prefix.length == 0.
            PrefixCrc removed_prefix;

            // A deque of (length, crc) pairs, representing length and crc of a prefix
            // of the Cord, before removed_prefix has been subtracted. The lengths of
            // the prefixes are stored in increasing order. If the Cord is not empty,
            // the last value in deque is the contains the CRC32C of the entire Cord
            // when removed_prefix is subtracted from it.
            std::deque<PrefixCrc> prefix_crc;
        };

        // Returns a reference to the representation of the chunked CRC32C data.
        const Rep &rep() const { return refcounted_rep_->rep; }

        // Returns a mutable reference to the representation of the chunked CRC32C
        // data. Calling this function will copy the data if another instance also
        // holds a reference to the data, so it is important to call rep() instead if
        // the data may not be mutated.
        Rep *mutable_rep() {
            if (refcounted_rep_->count.load(std::memory_order_acquire) != 1) {
                RefcountedRep *copy = new RefcountedRep;
                copy->rep = refcounted_rep_->rep;
                Unref(refcounted_rep_);
                refcounted_rep_ = copy;
            }
            return &refcounted_rep_->rep;
        }

        // Returns the CRC32C of the entire Cord.
        turbo::CRC32C Checksum() const;

        // Returns true if the chunked CRC32C cached is normalized.
        bool IsNormalized() const { return rep().removed_prefix.length == 0; }

        // Normalizes the chunked CRC32C checksum cache by subtracting any removed
        // prefix from the chunks.
        void Normalize();

        // Returns the number of cached chunks.
        size_t NumChunks() const { return rep().prefix_crc.size(); }

        // Helper that returns the (length, crc) of the `n`-th cached chunked.
        PrefixCrc NormalizedPrefixCrcAtNthChunk(size_t n) const;

        // Poisons all chunks to so that Checksum() will likely be incorrect with high
        // probability.
        void Poison();

    private:
        struct RefcountedRep {
            std::atomic<int32_t> count{1};
            Rep rep;
        };

        // Adds a reference to the shared global empty `RefcountedRep`, and returns a
        // pointer to the `RefcountedRep`. This is an optimization to avoid unneeded
        // allocations when the allocation is unlikely to ever be used. The returned
        // pointer can be `Unref()`ed when it is no longer needed.  Since the returned
        // instance will always have a reference counter greater than 1, attempts to
        // modify it (by calling `mutable_rep()`) will create a new unshared copy.
        static RefcountedRep *RefSharedEmptyRep();

        static void Ref(RefcountedRep *r) {
            assert(r != nullptr);
            r->count.fetch_add(1, std::memory_order_relaxed);
        }

        static void Unref(RefcountedRep *r) {
            assert(r != nullptr);
            if (r->count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                delete r;
            }
        }

        RefcountedRep *refcounted_rep_;
    };

}  // namespace turbo::crc_internal
