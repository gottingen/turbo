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

#include <turbo/crypto/internal/crc_cord_state.h>

#include <cassert>

#include <turbo/base/config.h>
#include <turbo/base/no_destructor.h>
#include <turbo/numeric/bits.h>

namespace turbo::crc_internal {

    CrcCordState::RefcountedRep *CrcCordState::RefSharedEmptyRep() {
        static turbo::NoDestructor<CrcCordState::RefcountedRep> empty;

        assert(empty->count.load(std::memory_order_relaxed) >= 1);
        assert(empty->rep.removed_prefix.length == 0);
        assert(empty->rep.prefix_crc.empty());

        Ref(empty.get());
        return empty.get();
    }

    CrcCordState::CrcCordState() : refcounted_rep_(new RefcountedRep) {}

    CrcCordState::CrcCordState(const CrcCordState &other)
            : refcounted_rep_(other.refcounted_rep_) {
        Ref(refcounted_rep_);
    }

    CrcCordState::CrcCordState(CrcCordState &&other)
            : refcounted_rep_(other.refcounted_rep_) {
        // Make `other` valid for use after move.
        other.refcounted_rep_ = RefSharedEmptyRep();
    }

    CrcCordState &CrcCordState::operator=(const CrcCordState &other) {
        if (this != &other) {
            Unref(refcounted_rep_);
            refcounted_rep_ = other.refcounted_rep_;
            Ref(refcounted_rep_);
        }
        return *this;
    }

    CrcCordState &CrcCordState::operator=(CrcCordState &&other) {
        if (this != &other) {
            Unref(refcounted_rep_);
            refcounted_rep_ = other.refcounted_rep_;
            // Make `other` valid for use after move.
            other.refcounted_rep_ = RefSharedEmptyRep();
        }
        return *this;
    }

    CrcCordState::~CrcCordState() {
        Unref(refcounted_rep_);
    }

    CRC32C CrcCordState::Checksum() const {
        if (rep().prefix_crc.empty()) {
            return turbo::CRC32C{0};
        }
        if (IsNormalized()) {
            return rep().prefix_crc.back().crc;
        }
        return turbo::remove_crc32c_prefix(
                rep().removed_prefix.crc, rep().prefix_crc.back().crc,
                rep().prefix_crc.back().length - rep().removed_prefix.length);
    }

    CrcCordState::PrefixCrc CrcCordState::NormalizedPrefixCrcAtNthChunk(
            size_t n) const {
        assert(n < NumChunks());
        if (IsNormalized()) {
            return rep().prefix_crc[n];
        }
        size_t length = rep().prefix_crc[n].length - rep().removed_prefix.length;
        return PrefixCrc(length,
                         turbo::remove_crc32c_prefix(rep().removed_prefix.crc,
                                                     rep().prefix_crc[n].crc, length));
    }

    void CrcCordState::Normalize() {
        if (IsNormalized() || rep().prefix_crc.empty()) {
            return;
        }

        Rep *r = mutable_rep();
        for (auto &prefix_crc: r->prefix_crc) {
            size_t remaining = prefix_crc.length - r->removed_prefix.length;
            prefix_crc.crc = turbo::remove_crc32c_prefix(r->removed_prefix.crc,
                                                         prefix_crc.crc, remaining);
            prefix_crc.length = remaining;
        }
        r->removed_prefix = PrefixCrc();
    }

    void CrcCordState::Poison() {
        Rep *rep = mutable_rep();
        if (NumChunks() > 0) {
            for (auto &prefix_crc: rep->prefix_crc) {
                // This is basically CRC32::Scramble().
                uint32_t crc = static_cast<uint32_t>(prefix_crc.crc);
                crc += 0x2e76e41b;
                crc = turbo::rotr(crc, 17);
                prefix_crc.crc = CRC32C{crc};
            }
        } else {
            // Add a fake corrupt chunk.
            rep->prefix_crc.emplace_back(0, CRC32C{1});
        }
    }

}  // namespace turbo::crc_internal
