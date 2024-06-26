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

#include <turbo/crypto/crc32c.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

#include <gtest/gtest.h>
#include <turbo/crypto/internal/crc32c.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/str_format.h>
#include <turbo/strings/string_view.h>

namespace {

TEST(CRC32C, RFC3720) {
  // Test the results of the vectors from
  // https://www.rfc-editor.org/rfc/rfc3720#appendix-B.4
  char data[32];

  // 32 bytes of ones.
  memset(data, 0, sizeof(data));
  EXPECT_EQ(turbo::compute_crc32c(std::string_view(data, sizeof(data))),
            turbo::CRC32C{0x8a9136aa});

  // 32 bytes of ones.
  memset(data, 0xff, sizeof(data));
  EXPECT_EQ(turbo::compute_crc32c(std::string_view(data, sizeof(data))),
            turbo::CRC32C{0x62a8ab43});

  // 32 incrementing bytes.
  for (int i = 0; i < 32; ++i) data[i] = static_cast<char>(i);
  EXPECT_EQ(turbo::compute_crc32c(std::string_view(data, sizeof(data))),
            turbo::CRC32C{0x46dd794e});

  // 32 decrementing bytes.
  for (int i = 0; i < 32; ++i) data[i] = static_cast<char>(31 - i);
  EXPECT_EQ(turbo::compute_crc32c(std::string_view(data, sizeof(data))),
            turbo::CRC32C{0x113fdb5c});

  // An iSCSI - SCSI Read (10) Command PDU.
  constexpr uint8_t cmd[48] = {
      0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x18, 0x28, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  EXPECT_EQ(turbo::compute_crc32c(std::string_view(
                reinterpret_cast<const char*>(cmd), sizeof(cmd))),
            turbo::CRC32C{0xd9963a56});
}

std::string TestString(size_t len) {
  std::string result;
  result.reserve(len);
  for (size_t i = 0; i < len; ++i) {
    result.push_back(static_cast<char>(i % 256));
  }
  return result;
}

TEST(CRC32C, Compute) {
  EXPECT_EQ(turbo::compute_crc32c(""), turbo::CRC32C{0});
  EXPECT_EQ(turbo::compute_crc32c("hello world"), turbo::CRC32C{0xc99465aa});
}

TEST(CRC32C, Extend) {
  uint32_t base = 0xC99465AA;  // CRC32C of "Hello World"
  std::string extension = "Extension String";

  EXPECT_EQ(
      turbo::extend_crc32c(turbo::CRC32C{base}, extension),
      turbo::CRC32C{0xD2F65090});  // CRC32C of "Hello WorldExtension String"
}

TEST(CRC32C, ExtendByZeroes) {
  std::string base = "hello world";
  turbo::CRC32C base_crc = turbo::CRC32C{0xc99465aa};

  constexpr size_t kExtendByValues[] = {100, 10000, 100000};
  for (const size_t extend_by : kExtendByValues) {
    SCOPED_TRACE(extend_by);
    turbo::CRC32C crc2 = turbo::extend_crc32c_by_zeroes(base_crc, extend_by);
    EXPECT_EQ(crc2, turbo::compute_crc32c(base + std::string(extend_by, '\0')));
  }
}

TEST(CRC32C, UnextendByZeroes) {
  constexpr size_t kExtendByValues[] = {2, 200, 20000, 200000, 20000000};
  constexpr size_t kUnextendByValues[] = {0, 100, 10000, 100000, 10000000};

  for (auto seed_crc : {turbo::CRC32C{0}, turbo::CRC32C{0xc99465aa}}) {
    SCOPED_TRACE(seed_crc);
    for (const size_t size_1 : kExtendByValues) {
      for (const size_t size_2 : kUnextendByValues) {
        size_t extend_size = std::max(size_1, size_2);
        size_t unextend_size = std::min(size_1, size_2);
        SCOPED_TRACE(extend_size);
        SCOPED_TRACE(unextend_size);

        // Extending by A zeroes an unextending by B<A zeros should be identical
        // to extending by A-B zeroes.
        turbo::CRC32C crc1 = seed_crc;
        crc1 = turbo::extend_crc32c_by_zeroes(crc1, extend_size);
        crc1 = turbo::crc_internal::UnextendCrc32cByZeroes(crc1, unextend_size);

        turbo::CRC32C crc2 = seed_crc;
        crc2 = turbo::extend_crc32c_by_zeroes(crc2, extend_size - unextend_size);

        EXPECT_EQ(crc1, crc2);
      }
    }
  }

  constexpr size_t kSizes[] = {0, 1, 100, 10000};
  for (const size_t size : kSizes) {
    SCOPED_TRACE(size);
    std::string string_before = TestString(size);
    std::string string_after = string_before + std::string(size, '\0');

    turbo::CRC32C crc_before = turbo::compute_crc32c(string_before);
    turbo::CRC32C crc_after = turbo::compute_crc32c(string_after);

    EXPECT_EQ(crc_before,
              turbo::crc_internal::UnextendCrc32cByZeroes(crc_after, size));
  }
}

TEST(CRC32C, Concat) {
  std::string hello = "Hello, ";
  std::string world = "world!";
  std::string hello_world = turbo::str_cat(hello, world);

  turbo::CRC32C crc_a = turbo::compute_crc32c(hello);
  turbo::CRC32C crc_b = turbo::compute_crc32c(world);
  turbo::CRC32C crc_ab = turbo::compute_crc32c(hello_world);

  EXPECT_EQ(turbo::concat_crc32c(crc_a, crc_b, world.size()), crc_ab);
}

TEST(CRC32C, Memcpy) {
  constexpr size_t kBytesSize[] = {0, 1, 20, 500, 100000};
  for (size_t bytes : kBytesSize) {
    SCOPED_TRACE(bytes);
    std::string sample_string = TestString(bytes);
    std::string target_buffer = std::string(bytes, '\0');

    turbo::CRC32C memcpy_crc =
        turbo::memcpy_crc32c(&(target_buffer[0]), sample_string.data(), bytes);
    turbo::CRC32C compute_crc = turbo::compute_crc32c(sample_string);

    EXPECT_EQ(memcpy_crc, compute_crc);
    EXPECT_EQ(sample_string, target_buffer);
  }
}

TEST(CRC32C, remove_prefix) {
  std::string hello = "Hello, ";
  std::string world = "world!";
  std::string hello_world = turbo::str_cat(hello, world);

  turbo::CRC32C crc_a = turbo::compute_crc32c(hello);
  turbo::CRC32C crc_b = turbo::compute_crc32c(world);
  turbo::CRC32C crc_ab = turbo::compute_crc32c(hello_world);

  EXPECT_EQ(turbo::remove_crc32c_prefix(crc_a, crc_ab, world.size()), crc_b);
}

TEST(CRC32C, remove_suffix) {
  std::string hello = "Hello, ";
  std::string world = "world!";
  std::string hello_world = turbo::str_cat(hello, world);

  turbo::CRC32C crc_a = turbo::compute_crc32c(hello);
  turbo::CRC32C crc_b = turbo::compute_crc32c(world);
  turbo::CRC32C crc_ab = turbo::compute_crc32c(hello_world);

  EXPECT_EQ(turbo::remove_crc32c_suffix(crc_ab, crc_b, world.size()), crc_a);
}

TEST(CRC32C, InsertionOperator) {
  {
    std::ostringstream buf;
    buf << turbo::CRC32C{0xc99465aa};
    EXPECT_EQ(buf.str(), "c99465aa");
  }
  {
    std::ostringstream buf;
    buf << turbo::CRC32C{0};
    EXPECT_EQ(buf.str(), "00000000");
  }
  {
    std::ostringstream buf;
    buf << turbo::CRC32C{17};
    EXPECT_EQ(buf.str(), "00000011");
  }
}

TEST(CRC32C, turbo_stringify) {
  // str_format
  EXPECT_EQ(turbo::str_format("%v", turbo::CRC32C{0xc99465aa}), "c99465aa");
  EXPECT_EQ(turbo::str_format("%v", turbo::CRC32C{0}), "00000000");
  EXPECT_EQ(turbo::str_format("%v", turbo::CRC32C{17}), "00000011");

  // str_cat
  EXPECT_EQ(turbo::str_cat(turbo::CRC32C{0xc99465aa}), "c99465aa");
  EXPECT_EQ(turbo::str_cat(turbo::CRC32C{0}), "00000000");
  EXPECT_EQ(turbo::str_cat(turbo::CRC32C{17}), "00000011");
}

}  // namespace
