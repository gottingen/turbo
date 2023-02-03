//
// Created by 李寅斌 on 2023/2/3.
//

#include <turbo/base/uuid.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

void test_threaded_uuid(size_t N) {

  std::vector<turbo::UUID> uuids(65536);

  // threaded
  std::mutex mutex;
  std::vector<std::thread> threads;

  for(size_t i=0; i<N; ++i) {
    threads.emplace_back([&](){
      for(int i=0; i<1000; ++i) {
        std::lock_guard<std::mutex> lock(mutex);
        uuids.push_back(turbo::UUID());
      }
    });
  }

  for(auto& t : threads) {
    t.join();
  }

  auto size = uuids.size();
  std::sort(uuids.begin(), uuids.end());
  std::unique(uuids.begin(), uuids.end());
  EXPECT_TRUE(uuids.size() == size);
}


TEST(uuid, all) {

  turbo::UUID u1, u2, u3, u4;

  // Comparator.
  EXPECT_TRUE(u1 == u1);

  // Copy
  u2 = u1;
  EXPECT_TRUE(u1 == u2);

  // Move
  u3 = std::move(u1);
  EXPECT_TRUE(u2 == u3);

  // Copy constructor
  turbo::UUID u5(u4);
  EXPECT_TRUE(u5 == u4);

  // Move constructor.
  turbo::UUID u6(std::move(u4));
  EXPECT_TRUE(u5 == u6);

  // Uniqueness
  std::vector<turbo::UUID> uuids(65536);
  std::sort(uuids.begin(), uuids.end());
  std::unique(uuids.begin(), uuids.end());
  EXPECT_TRUE(uuids.size() == 65536);

}

TEST(uuid, 10threads) {
  test_threaded_uuid(10);
}

TEST(uuid, 100threads) {
  test_threaded_uuid(100);
}

