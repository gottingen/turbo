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

// -----------------------------------------------------------------------------
// File: internal/proto.h
// -----------------------------------------------------------------------------
//
// Declares functions for serializing and deserializing data to and from memory
// buffers in protocol buffer wire format.  This library takes no steps to
// ensure that the encoded data matches with any message specification.

#ifndef TURBO_LOG_INTERNAL_PROTO_H_
#define TURBO_LOG_INTERNAL_PROTO_H_

#include <cstddef>
#include <cstdint>
#include <limits>

#include <turbo/base/attributes.h>
#include <turbo/base/casts.h>
#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/span.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

// turbo::Span<char> represents a view into the available space in a mutable
// buffer during encoding.  Encoding functions shrink the span as they go so
// that the same view can be passed to a series of Encode functions.  If the
// data do not fit, nothing is encoded, the view is set to size zero (so that
// all subsequent encode calls fail), and false is returned.  Otherwise true is
// returned.

// In particular, attempting to encode a series of data into an insufficient
// buffer has consistent and efficient behavior without any caller-side error
// checking.  Individual values will be encoded in their entirety or not at all
// (unless one of the `Truncate` functions is used).  Once a value is omitted
// because it does not fit, no subsequent values will be encoded to preserve
// ordering; the decoded sequence will be a prefix of the original sequence.

// There are two ways to encode a message-typed field:
//
// * Construct its contents in a separate buffer and use `EncodeBytes` to copy
//   it into the primary buffer with type, tag, and length.
// * Use `EncodeMessageStart` to write type and tag fields and reserve space for
//   the length field, then encode the contents directly into the buffer, then
//   use `EncodeMessageLength` to write the actual length into the reserved
//   bytes.  This works fine if the actual length takes fewer bytes to encode
//   than were reserved, although you don't get your extra bytes back.
//   This approach will always produce a valid encoding, but your protocol may
//   require that the whole message field by omitted if the buffer is too small
//   to contain all desired subfields.  In this case, operate on a copy of the
//   buffer view and assign back only if everything fit, i.e. if the last
//   `Encode` call returned true.

// Encodes the specified integer as a varint field and returns true if it fits.
// Used for int32_t, int64_t, uint32_t, uint64_t, bool, and enum field types.
// Consumes up to kMaxVarintSize * 2 bytes (20).
bool EncodeVarint(uint64_t tag, uint64_t value, turbo::Span<char> *buf);
inline bool EncodeVarint(uint64_t tag, int64_t value, turbo::Span<char> *buf) {
  return EncodeVarint(tag, static_cast<uint64_t>(value), buf);
}
inline bool EncodeVarint(uint64_t tag, uint32_t value, turbo::Span<char> *buf) {
  return EncodeVarint(tag, static_cast<uint64_t>(value), buf);
}
inline bool EncodeVarint(uint64_t tag, int32_t value, turbo::Span<char> *buf) {
  return EncodeVarint(tag, static_cast<uint64_t>(value), buf);
}

// Encodes the specified integer as a varint field using ZigZag encoding and
// returns true if it fits.
// Used for sint32 and sint64 field types.
// Consumes up to kMaxVarintSize * 2 bytes (20).
inline bool EncodeVarintZigZag(uint64_t tag, int64_t value,
                               turbo::Span<char> *buf) {
  if (value < 0)
    return EncodeVarint(tag, 2 * static_cast<uint64_t>(-(value + 1)) + 1, buf);
  return EncodeVarint(tag, 2 * static_cast<uint64_t>(value), buf);
}

// Encodes the specified integer as a 64-bit field and returns true if it fits.
// Used for fixed64 and sfixed64 field types.
// Consumes up to kMaxVarintSize + 8 bytes (18).
bool Encode64Bit(uint64_t tag, uint64_t value, turbo::Span<char> *buf);
inline bool Encode64Bit(uint64_t tag, int64_t value, turbo::Span<char> *buf) {
  return Encode64Bit(tag, static_cast<uint64_t>(value), buf);
}
inline bool Encode64Bit(uint64_t tag, uint32_t value, turbo::Span<char> *buf) {
  return Encode64Bit(tag, static_cast<uint64_t>(value), buf);
}
inline bool Encode64Bit(uint64_t tag, int32_t value, turbo::Span<char> *buf) {
  return Encode64Bit(tag, static_cast<uint64_t>(value), buf);
}

// Encodes the specified double as a 64-bit field and returns true if it fits.
// Used for double field type.
// Consumes up to kMaxVarintSize + 8 bytes (18).
inline bool EncodeDouble(uint64_t tag, double value, turbo::Span<char> *buf) {
  return Encode64Bit(tag, turbo::bit_cast<uint64_t>(value), buf);
}

// Encodes the specified integer as a 32-bit field and returns true if it fits.
// Used for fixed32 and sfixed32 field types.
// Consumes up to kMaxVarintSize + 4 bytes (14).
bool Encode32Bit(uint64_t tag, uint32_t value, turbo::Span<char> *buf);
inline bool Encode32Bit(uint64_t tag, int32_t value, turbo::Span<char> *buf) {
  return Encode32Bit(tag, static_cast<uint32_t>(value), buf);
}

// Encodes the specified float as a 32-bit field and returns true if it fits.
// Used for float field type.
// Consumes up to kMaxVarintSize + 4 bytes (14).
inline bool EncodeFloat(uint64_t tag, float value, turbo::Span<char> *buf) {
  return Encode32Bit(tag, turbo::bit_cast<uint32_t>(value), buf);
}

// Encodes the specified bytes as a length-delimited field and returns true if
// they fit.
// Used for string, bytes, message, and packed-repeated field type.
// Consumes up to kMaxVarintSize * 2 + value.size() bytes (20 + value.size()).
bool EncodeBytes(uint64_t tag, turbo::Span<const char> value,
                 turbo::Span<char> *buf);

// Encodes as many of the specified bytes as will fit as a length-delimited
// field and returns true as long as the field header (`tag_type` and `length`)
// fits.
// Used for string, bytes, message, and packed-repeated field type.
// Consumes up to kMaxVarintSize * 2 + value.size() bytes (20 + value.size()).
bool EncodeBytesTruncate(uint64_t tag, turbo::Span<const char> value,
                         turbo::Span<char> *buf);

// Encodes the specified string as a length-delimited field and returns true if
// it fits.
// Used for string, bytes, message, and packed-repeated field type.
// Consumes up to kMaxVarintSize * 2 + value.size() bytes (20 + value.size()).
inline bool EncodeString(uint64_t tag, turbo::string_view value,
                         turbo::Span<char> *buf) {
  return EncodeBytes(tag, value, buf);
}

// Encodes as much of the specified string as will fit as a length-delimited
// field and returns true as long as the field header (`tag_type` and `length`)
// fits.
// Used for string, bytes, message, and packed-repeated field type.
// Consumes up to kMaxVarintSize * 2 + value.size() bytes (20 + value.size()).
inline bool EncodeStringTruncate(uint64_t tag, turbo::string_view value,
                                 turbo::Span<char> *buf) {
  return EncodeBytesTruncate(tag, value, buf);
}

// Encodes the header for a length-delimited field containing up to `max_size`
// bytes or the number remaining in the buffer, whichever is less.  If the
// header fits, a non-nullptr `Span` is returned; this must be passed to
// `EncodeMessageLength` after all contents are encoded to finalize the length
// field.  If the header does not fit, a nullptr `Span` is returned which is
// safe to pass to `EncodeMessageLength` but need not be.
// Used for string, bytes, message, and packed-repeated field type.
// Consumes up to kMaxVarintSize * 2 bytes (20).
TURBO_MUST_USE_RESULT turbo::Span<char> EncodeMessageStart(uint64_t tag,
                                                         uint64_t max_size,
                                                         turbo::Span<char> *buf);

// Finalizes the length field in `msg` so that it encompasses all data encoded
// since the call to `EncodeMessageStart` which returned `msg`.  Does nothing if
// `msg` is a `nullptr` `Span`.
void EncodeMessageLength(turbo::Span<char> msg, const turbo::Span<char> *buf);

enum class WireType : uint64_t {
  kVarint = 0,
  k64Bit = 1,
  kLengthDelimited = 2,
  k32Bit = 5,
};

constexpr size_t VarintSize(uint64_t value) {
  return value < 128 ? 1 : 1 + VarintSize(value >> 7);
}
constexpr size_t MinVarintSize() {
  return VarintSize((std::numeric_limits<uint64_t>::min)());
}
constexpr size_t MaxVarintSize() {
  return VarintSize((std::numeric_limits<uint64_t>::max)());
}

constexpr uint64_t MaxVarintForSize(size_t size) {
  return size >= 10 ? (std::numeric_limits<uint64_t>::max)()
                    : (static_cast<uint64_t>(1) << size * 7) - 1;
}

// `BufferSizeFor` returns a number of bytes guaranteed to be sufficient to
// store encoded fields of the specified WireTypes regardless of tag numbers and
// data values.  This only makes sense for `WireType::kLengthDelimited` if you
// add in the length of the contents yourself, e.g. for string and bytes fields
// by adding the lengths of any encoded strings to the return value or for
// submessage fields by enumerating the fields you may encode into their
// contents.
constexpr size_t BufferSizeFor() { return 0; }
template <typename... T>
constexpr size_t BufferSizeFor(WireType type, T... tail) {
  // tag_type + data + ...
  return MaxVarintSize() +
         (type == WireType::kVarint ? MaxVarintSize() :              //
              type == WireType::k64Bit ? 8 :                         //
                  type == WireType::k32Bit ? 4 : MaxVarintSize()) +  //
         BufferSizeFor(tail...);
}

// turbo::Span<const char> represents a view into the un-processed space in a
// buffer during decoding.  Decoding functions shrink the span as they go so
// that the same view can be decoded iteratively until all data are processed.
// In general, if the buffer is exhausted but additional bytes are expected by
// the decoder, it will return values as if the additional bytes were zeros.
// Length-delimited fields are an exception - if the encoded length field
// indicates more data bytes than are available in the buffer, the `bytes_value`
// and `string_value` accessors will return truncated views.

class ProtoField final {
 public:
  // Consumes bytes from `data` and returns true if there were any bytes to
  // decode.
  bool DecodeFrom(turbo::Span<const char> *data);
  uint64_t tag() const { return tag_; }
  WireType type() const { return type_; }

  // These value accessors will return nonsense if the data were not encoded in
  // the corresponding wiretype from the corresponding C++ (or other language)
  // type.

  double double_value() const { return turbo::bit_cast<double>(value_); }
  float float_value() const {
    return turbo::bit_cast<float>(static_cast<uint32_t>(value_));
  }
  int32_t int32_value() const { return static_cast<int32_t>(value_); }
  int64_t int64_value() const { return static_cast<int64_t>(value_); }
  int32_t sint32_value() const {
    if (value_ % 2) return static_cast<int32_t>(0 - ((value_ - 1) / 2) - 1);
    return static_cast<int32_t>(value_ / 2);
  }
  int64_t sint64_value() const {
    if (value_ % 2) return 0 - ((value_ - 1) / 2) - 1;
    return value_ / 2;
  }
  uint32_t uint32_value() const { return static_cast<uint32_t>(value_); }
  uint64_t uint64_value() const { return value_; }
  bool bool_value() const { return value_ != 0; }
  // To decode an enum, call int32_value() and cast to the appropriate type.
  // Note that the official C++ proto compiler treats enum fields with values
  // that do not correspond to a defined enumerator as unknown fields.

  // To decode fields within a submessage field, call
  // `DecodeNextField(field.BytesValue())`.
  turbo::Span<const char> bytes_value() const { return data_; }
  turbo::string_view string_value() const {
    const auto data = bytes_value();
    return turbo::string_view(data.data(), data.size());
  }
  // Returns the encoded length of a length-delimited field.  This equals
  // `bytes_value().size()` except when the latter has been truncated due to
  // buffer underrun.
  uint64_t encoded_length() const { return value_; }

 private:
  uint64_t tag_;
  WireType type_;
  // For `kTypeVarint`, `kType64Bit`, and `kType32Bit`, holds the decoded value.
  // For `kTypeLengthDelimited`, holds the decoded length.
  uint64_t value_;
  turbo::Span<const char> data_;
};

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_PROTO_H_
