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

// Tests for pointer utilities.

#include <turbo/memory/memory.h>

#include <sys/types.h>

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using ::testing::ElementsAre;
using ::testing::Return;

// This class creates observable behavior to verify that a destructor has
// been called, via the instance_count variable.
class DestructorVerifier {
 public:
  DestructorVerifier() { ++instance_count_; }
  DestructorVerifier(const DestructorVerifier&) = delete;
  DestructorVerifier& operator=(const DestructorVerifier&) = delete;
  ~DestructorVerifier() { --instance_count_; }

  // The number of instances of this class currently active.
  static int instance_count() { return instance_count_; }

 private:
  // The number of instances of this class currently active.
  static int instance_count_;
};

int DestructorVerifier::instance_count_ = 0;

TEST(WrapUniqueTest, WrapUnique) {
  // Test that the unique_ptr is constructed properly by verifying that the
  // destructor for its payload gets called at the proper time.
  {
    auto dv = new DestructorVerifier;
    EXPECT_EQ(1, DestructorVerifier::instance_count());
    std::unique_ptr<DestructorVerifier> ptr = turbo::WrapUnique(dv);
    EXPECT_EQ(1, DestructorVerifier::instance_count());
  }
  EXPECT_EQ(0, DestructorVerifier::instance_count());
}

// InitializationVerifier fills in a pattern when allocated so we can
// distinguish between its default and value initialized states (without
// accessing truly uninitialized memory).
struct InitializationVerifier {
  static constexpr int kDefaultScalar = 0x43;
  static constexpr int kDefaultArray = 0x4B;

  static void* operator new(size_t n) {
    void* ret = ::operator new(n);
    memset(ret, kDefaultScalar, n);
    return ret;
  }

  static void* operator new[](size_t n) {
    void* ret = ::operator new[](n);
    memset(ret, kDefaultArray, n);
    return ret;
  }

  int a;
  int b;
};

struct ArrayWatch {
  void* operator new[](size_t n) {
    allocs().push_back(n);
    return ::operator new[](n);
  }
  void operator delete[](void* p) { return ::operator delete[](p); }
  static std::vector<size_t>& allocs() {
    static auto& v = *new std::vector<size_t>;
    return v;
  }
};

TEST(RawPtrTest, RawPointer) {
  int i = 5;
  EXPECT_EQ(&i, turbo::RawPtr(&i));
}

TEST(RawPtrTest, SmartPointer) {
  int* o = new int(5);
  std::unique_ptr<int> p(o);
  EXPECT_EQ(o, turbo::RawPtr(p));
}

class IntPointerNonConstDeref {
 public:
  explicit IntPointerNonConstDeref(int* p) : p_(p) {}
  friend bool operator!=(const IntPointerNonConstDeref& a, std::nullptr_t) {
    return a.p_ != nullptr;
  }
  int& operator*() { return *p_; }

 private:
  std::unique_ptr<int> p_;
};

TEST(RawPtrTest, SmartPointerNonConstDereference) {
  int* o = new int(5);
  IntPointerNonConstDeref p(o);
  EXPECT_EQ(o, turbo::RawPtr(p));
}

TEST(RawPtrTest, NullValuedRawPointer) {
  int* p = nullptr;
  EXPECT_EQ(nullptr, turbo::RawPtr(p));
}

TEST(RawPtrTest, NullValuedSmartPointer) {
  std::unique_ptr<int> p;
  EXPECT_EQ(nullptr, turbo::RawPtr(p));
}

TEST(RawPtrTest, Nullptr) {
  auto p = turbo::RawPtr(nullptr);
  EXPECT_TRUE((std::is_same<std::nullptr_t, decltype(p)>::value));
  EXPECT_EQ(nullptr, p);
}

TEST(RawPtrTest, Null) {
  auto p = turbo::RawPtr(nullptr);
  EXPECT_TRUE((std::is_same<std::nullptr_t, decltype(p)>::value));
  EXPECT_EQ(nullptr, p);
}

TEST(RawPtrTest, Zero) {
  auto p = turbo::RawPtr(nullptr);
  EXPECT_TRUE((std::is_same<std::nullptr_t, decltype(p)>::value));
  EXPECT_EQ(nullptr, p);
}

TEST(ShareUniquePtrTest, Share) {
  auto up = turbo::make_unique<int>();
  int* rp = up.get();
  auto sp = turbo::ShareUniquePtr(std::move(up));
  EXPECT_EQ(sp.get(), rp);
}

TEST(ShareUniquePtrTest, ShareNull) {
  struct NeverDie {
    using pointer = void*;
    void operator()(pointer) {
      ASSERT_TRUE(false) << "Deleter should not have been called.";
    }
  };

  std::unique_ptr<void, NeverDie> up;
  auto sp = turbo::ShareUniquePtr(std::move(up));
}

TEST(WeakenPtrTest, Weak) {
  auto sp = std::make_shared<int>();
  auto wp = turbo::WeakenPtr(sp);
  EXPECT_EQ(sp.get(), wp.lock().get());
  sp.reset();
  EXPECT_TRUE(wp.expired());
}

// Should not compile.
/*
TEST(RawPtrTest, NotAPointer) {
  turbo::RawPtr(1.5);
}
*/

TEST(AllocatorNoThrowTest, DefaultAllocator) {
#if defined(TURBO_ALLOCATOR_NOTHROW) && TURBO_ALLOCATOR_NOTHROW
  EXPECT_TRUE(turbo::default_allocator_is_nothrow::value);
#else
  EXPECT_FALSE(turbo::default_allocator_is_nothrow::value);
#endif
}

TEST(AllocatorNoThrowTest, StdAllocator) {
#if defined(TURBO_ALLOCATOR_NOTHROW) && TURBO_ALLOCATOR_NOTHROW
  EXPECT_TRUE(turbo::allocator_is_nothrow<std::allocator<int>>::value);
#else
  EXPECT_FALSE(turbo::allocator_is_nothrow<std::allocator<int>>::value);
#endif
}

TEST(AllocatorNoThrowTest, CustomAllocator) {
  struct NoThrowAllocator {
    using is_nothrow = std::true_type;
  };
  struct CanThrowAllocator {
    using is_nothrow = std::false_type;
  };
  struct UnspecifiedAllocator {};
  EXPECT_TRUE(turbo::allocator_is_nothrow<NoThrowAllocator>::value);
  EXPECT_FALSE(turbo::allocator_is_nothrow<CanThrowAllocator>::value);
  EXPECT_FALSE(turbo::allocator_is_nothrow<UnspecifiedAllocator>::value);
}

}  // namespace
