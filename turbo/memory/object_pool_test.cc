
#include "gtest/gtest.h"
#include "turbo/memory/object_pool.h"
#include <turbo/meta/type_traits.h>
#include <turbo/base/uuid.h>
#include <turbo/meta/algorithm.h>

// --------------------------------------------------------
// Testcase: ObjectPool.Sequential
// --------------------------------------------------------
struct Poolable {
  std::string str;
  std::vector<int> vec;
  int a;
  char b;

  TURBO_ENABLE_POOLABLE_ON_THIS;
};

TEST(ObjectPool, Sequential) {

  for(unsigned w=1; w<=4; w++) {

    turbo::ObjectPool<Poolable> pool(w);

    EXPECT_TRUE(pool.num_heaps() > 0);
    EXPECT_TRUE(pool.num_local_heaps() > 0);
    EXPECT_TRUE(pool.num_global_heaps() > 0);
    EXPECT_TRUE(pool.num_bins_per_local_heap() > 0);
    EXPECT_TRUE(pool.num_objects_per_bin() > 0);
    EXPECT_TRUE(pool.num_objects_per_block() > 0);
    EXPECT_TRUE(pool.emptiness_threshold() > 0);

    // fill out all objects
    size_t N = 100*pool.num_objects_per_block();

    std::set<Poolable*> set;

    for(size_t i=0; i<N; ++i) {
      auto item = pool.animate();
      EXPECT_TRUE(set.find(item) == set.end());
      set.insert(item);
    }

    EXPECT_TRUE(set.size() == N);

    for(auto s : set) {
      pool.recycle(s);
    }

    EXPECT_TRUE(N == pool.capacity());
    EXPECT_TRUE(N == pool.num_available_objects());
    EXPECT_TRUE(0 == pool.num_allocated_objects());

    for(size_t i=0; i<N; ++i) {
      auto item = pool.animate();
      EXPECT_TRUE(set.find(item) != set.end());
    }

    EXPECT_TRUE(pool.num_available_objects() == 0);
    EXPECT_TRUE(pool.num_allocated_objects() == N);
  }
}

// --------------------------------------------------------
// Testcase: ObjectPool.Threaded
// --------------------------------------------------------

template <typename T>
void threaded_objectpool(unsigned W) {

  turbo::ObjectPool<T> pool;

  std::vector<std::thread> threads;

  for(unsigned w=0; w<W; ++w) {
    threads.emplace_back([&pool](){
      std::vector<T*> items;
      for(int i=0; i<65536; ++i) {
        auto item = pool.animate();
        items.push_back(item);
      }
      for(auto item : items) {
        pool.recycle(item);
      }
    });
  }

  for(auto& thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(pool.num_allocated_objects() == 0);
  EXPECT_TRUE(pool.num_available_objects() == pool.capacity());
}

TEST(ObjectPool, 1thread) {
  threaded_objectpool<Poolable>(1);
}

TEST(ObjectPool, 2threads) {
  threaded_objectpool<Poolable>(2);
}

TEST(ObjectPool, 3threads) {
  threaded_objectpool<Poolable>(3);
}

TEST(ObjectPool, 4threads)  {
  threaded_objectpool<Poolable>(4);
}

TEST(ObjectPool, 5threads)  {
  threaded_objectpool<Poolable>(5);
}

TEST(ObjectPool, 6threads)  {
  threaded_objectpool<Poolable>(6);
}

TEST(ObjectPool, 7threads)  {
  threaded_objectpool<Poolable>(7);
}

TEST(ObjectPool, 8threads)  {
  threaded_objectpool<Poolable>(8);
}

TEST(ObjectPool, 9threads)  {
  threaded_objectpool<Poolable>(9);
}

TEST(ObjectPool, 10threads)  {
  threaded_objectpool<Poolable>(10);
}

TEST(ObjectPool, 11threads)  {
  threaded_objectpool<Poolable>(11);
}

TEST(ObjectPool, 12threads)  {
  threaded_objectpool<Poolable>(12);
}

TEST(ObjectPool, 13threads)  {
  threaded_objectpool<Poolable>(13);
}

TEST(ObjectPool, 14threads)  {
  threaded_objectpool<Poolable>(14);
}

TEST(ObjectPool, 15threads)  {
  threaded_objectpool<Poolable>(15);
}

TEST(ObjectPool, 16threads)  {
  threaded_objectpool<Poolable>(16);
}
