//
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
// Created by jeff on 24-6-8.
//
#include <turbo/container/cache.h>

#include <gtest/gtest.h>

using namespace turbo;

TEST(FIFOCache, Simple_Test) {
    FIFOCache<int, int> fc(2);

    fc.put(1, 10);
    fc.put(2, 20);

    EXPECT_EQ(fc.size(), 2);
    EXPECT_EQ(*fc.get_or_die(1), 10);
    EXPECT_EQ(*fc.get_or_die(2), 20);

    fc.put(1, 30);
    EXPECT_EQ(fc.size(), 2);
    EXPECT_EQ(*fc.get_or_die(1), 30);

    fc.put(3, 30);
    EXPECT_THROW(fc.get_or_die(1), std::range_error);
    EXPECT_EQ(*fc.get_or_die(2), 20);
    EXPECT_EQ(*fc.get_or_die(3), 30);
}

TEST(FIFOCache, Missing_Value) {
    FIFOCache<int, int> fc(2);

    fc.put(1, 10);

    EXPECT_EQ(fc.size(), 1);
    EXPECT_EQ(*fc.get_or_die(1), 10);
    EXPECT_THROW(fc.get_or_die(2), std::range_error);
}

TEST(FIFOCache, Sequence_Test) {
    constexpr int TEST_SIZE = 10;
    FIFOCache<std::string, int> fc(TEST_SIZE);

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        fc.put(std::to_string('0' + i), static_cast<int>(i));
    }

    EXPECT_EQ(fc.size(), TEST_SIZE);

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_EQ(*fc.get_or_die(std::to_string('0' + i)), i);
    }

    // replace a half
    for (size_t i = 0; i < TEST_SIZE / 2; ++i) {
        fc.put(std::to_string('a' + i), static_cast<int>(i));
    }

    EXPECT_EQ(fc.size(), TEST_SIZE);

    for (size_t i = 0; i < TEST_SIZE / 2; ++i) {
        EXPECT_THROW(fc.get_or_die(std::to_string('0' + i)), std::range_error);
    }

    for (size_t i = 0; i < TEST_SIZE / 2; ++i) {
        EXPECT_EQ(*fc.get_or_die(std::to_string('a' + i)), i);
    }

    for (size_t i = TEST_SIZE / 2; i < TEST_SIZE; ++i) {
        EXPECT_EQ(*fc.get_or_die(std::to_string('0' + i)), i);
    }
}

TEST(FIFOCache, Remove_Test) {
    constexpr std::size_t TEST_SIZE = 10;
    FIFOCache<std::string, std::size_t> fc(TEST_SIZE);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        fc.put(std::to_string(i), i);
    }

    EXPECT_EQ(fc.size(), TEST_SIZE);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_TRUE(fc.remove(std::to_string(i)));
    }

    EXPECT_EQ(fc.size(), 0);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_FALSE(fc.remove(std::to_string(i)));
    }
}

TEST(FIFOCache, try_get) {
    constexpr std::size_t TEST_CASE{10};
    FIFOCache<std::string, std::size_t> cache{TEST_CASE};

    for (std::size_t i = 0; i < TEST_CASE; ++i) {
        cache.put(std::to_string(i), i);
    }

    for (std::size_t i = 0; i < TEST_CASE; ++i) {
        auto element = cache.try_get(std::to_string(i));
        EXPECT_TRUE(element.second);
        EXPECT_EQ(*element.first, i);
    }

    for (std::size_t i = TEST_CASE; i < TEST_CASE * 2; ++i) {
        auto element = cache.try_get(std::to_string(i));
        EXPECT_FALSE(element.second);
    }
}

TEST(FIFOCache, GetWithReplacement) {
    FIFOCache<std::string, std::size_t> cache{2};

    cache.put("1", 1);
    cache.put("2", 2);

    auto element1 = cache.get_or_die("1");
    auto element2 = cache.get_or_die("2");
    EXPECT_EQ(*element1, 1);
    EXPECT_EQ(*element2, 2);
    cache.put("3", 3);
    auto element3 = cache.get_or_die("3");
    EXPECT_EQ(*element3, 3);

    std::string replaced_key;

    for (size_t i = 1; i <= 2; ++i) {
        const auto key = std::to_string(i);

        if (!cache.contains(key)) {
            replaced_key = key;
        }
    }

    EXPECT_FALSE(cache.contains(replaced_key));
    EXPECT_FALSE(cache.try_get(replaced_key).second);
    EXPECT_THROW(cache.get_or_die(replaced_key), std::range_error);
    EXPECT_EQ(*element1, 1);
    EXPECT_EQ(*element2, 2);
    EXPECT_EQ(*element3, 3);
}

TEST(FIFOCache, InvalidSize) {
    using test_type = FIFOCache<std::string, int>;
    EXPECT_THROW(test_type cache{0}, std::invalid_argument);
}


TEST(LFUCache, Simple_Test) {
    constexpr size_t FIRST_FREQ = 10;
    constexpr size_t SECOND_FREQ = 9;
    constexpr size_t THIRD_FREQ = 8;
    LFUCache<std::string, int> cache(3);

    cache.put("A", 1);
    cache.put("B", 2);
    cache.put("C", 3);

    for (size_t i = 0; i < FIRST_FREQ; ++i) {
        EXPECT_EQ(*cache.get_or_die("B"), 2);
    }

    for (size_t i = 0; i < SECOND_FREQ; ++i) {
        EXPECT_EQ(*cache.get_or_die("C"), 3);
    }

    for (size_t i = 0; i < THIRD_FREQ; ++i) {
        EXPECT_EQ(*cache.get_or_die("A"), 1);
    }

    cache.put("D", 4);

    EXPECT_EQ(*cache.get_or_die("B"), 2);
    EXPECT_EQ(*cache.get_or_die("C"), 3);
    EXPECT_EQ(*cache.get_or_die("D"), 4);
    EXPECT_THROW(cache.get_or_die("A"), std::range_error);
}

TEST(LFUCache, Single_Slot) {
    constexpr size_t TEST_SIZE = 5;
    LFUCache<int, int> cache(1);

    cache.put(1, 10);

    for (size_t i = 0; i < TEST_SIZE; ++i) {
        cache.put(1, static_cast<int>(i));
    }

    EXPECT_EQ(*cache.get_or_die(1), 4);

    cache.put(2, 20);

    EXPECT_THROW(cache.get_or_die(1), std::range_error);
    EXPECT_EQ(*cache.get_or_die(2), 20);
}

TEST(LFUCache, FrequencyIssue) {
    constexpr size_t TEST_SIZE = 50;
    LFUCache<int, int> cache(3);

    cache.put(1, 10);
    cache.put(2, 1);
    cache.put(3, 2);

    // cache value with key '1' will have the counter 50
    for (size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_NO_THROW(cache.get_or_die(1));
    }

    cache.put(4, 3);
    cache.put(5, 4);

    EXPECT_EQ(*cache.get_or_die(1), 10);
    EXPECT_EQ(*cache.get_or_die(2), 1);
    EXPECT_EQ(*cache.get_or_die(5), 4);
    EXPECT_THROW(cache.get_or_die(3), std::range_error);
    EXPECT_THROW(cache.get_or_die(4), std::range_error);

    cache.put(6, 5);
    cache.put(7, 6);

    EXPECT_EQ(*cache.get_or_die(1), 10);
    EXPECT_EQ(*cache.get_or_die(5), 4);
    EXPECT_EQ(*cache.get_or_die(7), 6);
    EXPECT_THROW(cache.get_or_die(3), std::range_error);
    EXPECT_THROW(cache.get_or_die(6), std::range_error);
}

TEST(LFUCache, Remove_Test) {
    constexpr std::size_t TEST_SIZE = 10;
    LFUCache<std::string, std::size_t> fc(TEST_SIZE);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        fc.put(std::to_string(i), i);
    }

    EXPECT_EQ(fc.size(), TEST_SIZE);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_TRUE(fc.remove(std::to_string(i)));
    }

    EXPECT_EQ(fc.size(), 0);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_FALSE(fc.remove(std::to_string(i)));
    }
}

TEST(LFUCache, try_get) {
    constexpr std::size_t TEST_CASE{10};
    LFUCache<std::string, std::size_t> cache{TEST_CASE};

    for (std::size_t i = 0; i < TEST_CASE; ++i) {
        cache.put(std::to_string(i), i);
    }

    for (std::size_t i = 0; i < TEST_CASE; ++i) {
        auto element = cache.try_get(std::to_string(i));
        EXPECT_TRUE(element.second);
        EXPECT_EQ(*element.first, i);
    }

    for (std::size_t i = TEST_CASE; i < TEST_CASE * 2; ++i) {
        auto element = cache.try_get(std::to_string(i));
        EXPECT_FALSE(element.second);
    }
}

TEST(LFUCache, GetWithReplacement) {
    LFUCache<std::string, std::size_t> cache{2};

    cache.put("1", 1);
    cache.put("2", 2);

    auto element1 = cache.get_or_die("1");
    auto element2 = cache.get_or_die("2");
    EXPECT_EQ(*element1, 1);
    EXPECT_EQ(*element2, 2);
    cache.put("3", 3);
    auto element3 = cache.get_or_die("3");
    EXPECT_EQ(*element3, 3);

    std::string replaced_key;

    for (size_t i = 1; i <= 2; ++i) {
        const auto key = std::to_string(i);

        if (!cache.contains(key)) {
            replaced_key = key;
        }
    }

    EXPECT_FALSE(cache.contains(replaced_key));
    EXPECT_FALSE(cache.try_get(replaced_key).second);
    EXPECT_THROW(cache.get_or_die(replaced_key), std::range_error);
    EXPECT_EQ(*element1, 1);
    EXPECT_EQ(*element2, 2);
    EXPECT_EQ(*element3, 3);
}

TEST(LFUCache, InvalidSize) {
    using test_type = LFUCache<std::string, int>;
    EXPECT_THROW(test_type cache{0}, std::invalid_argument);
}


TEST(CacheTest, SimplePut) {
    LRUCache<std::string, int> cache(1);

    cache.put("test", 666);

    EXPECT_EQ(*cache.get_or_die("test"), 666);
}

TEST(CacheTest, PutWithUpdate) {
    constexpr std::size_t TEST_CASE = 4;
    LRUCache<std::string, std::size_t> cache{TEST_CASE};

    for (size_t i = 0; i < TEST_CASE; ++i) {
        cache.put(std::to_string(i), i);

        const auto value = cache.get_or_die(std::to_string(i));
        ASSERT_EQ(i, *value);
    }

    for (size_t i = 0; i < TEST_CASE; ++i) {
        ASSERT_TRUE(cache.contains(std::to_string(i)));
        cache.put(std::to_string(i), i * 10);

        const auto value = cache.get_or_die(std::to_string(i));
        ASSERT_EQ(i * 10, *value);
    }
}

TEST(CacheTest, MissingValue) {
    LRUCache<std::string, int> cache(1);

    EXPECT_THROW(cache.get_or_die("test"), std::range_error);
}

TEST(CacheTest, KeepsAllValuesWithinCapacity) {
    constexpr int CACHE_CAP = 50;
    const int TEST_RECORDS = 100;
    LRUCache<int, int> cache(CACHE_CAP);

    for (int i = 0; i < TEST_RECORDS; ++i) {
        cache.put(i, i);
    }

    for (int i = 0; i < TEST_RECORDS - CACHE_CAP; ++i) {
        EXPECT_THROW(cache.get_or_die(i), std::range_error);
    }

    for (int i = TEST_RECORDS - CACHE_CAP; i < TEST_RECORDS; ++i) {
        EXPECT_EQ(i, *cache.get_or_die(i));
    }
}

TEST(LRUCache, Remove_Test) {
    constexpr std::size_t TEST_SIZE = 10;
    LRUCache<std::string, std::size_t> fc(TEST_SIZE);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        fc.put(std::to_string(i), i);
    }

    EXPECT_EQ(fc.size(), TEST_SIZE);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_TRUE(fc.remove(std::to_string(i)));
    }

    EXPECT_EQ(fc.size(), 0);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_FALSE(fc.remove(std::to_string(i)));
    }

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        fc.put(std::to_string(i), i);
    }
    EXPECT_TRUE(fc.remove(std::to_string(5)));
    EXPECT_FALSE(fc.remove(std::to_string(5)));
    EXPECT_FALSE(fc.contains(std::to_string(5)));


}

TEST(LRUCache, CachedCheck) {
    constexpr std::size_t TEST_SUITE = 4;
    LRUCache<std::string, std::size_t> cache(TEST_SUITE);

    for (std::size_t i = 0; i < TEST_SUITE; ++i) {
        cache.put(std::to_string(i), i);
    }

    for (std::size_t i = 0; i < TEST_SUITE; ++i) {
        EXPECT_TRUE(cache.contains(std::to_string(i)));
    }

    for (std::size_t i = TEST_SUITE; i < TEST_SUITE * 2; ++i) {
        EXPECT_FALSE(cache.contains(std::to_string(i)));
    }
}

TEST(LRUCache, ConstructCache) {
    EXPECT_THROW((LRUCache<std::string, std::size_t>(0)), std::invalid_argument);
    EXPECT_NO_THROW((LRUCache<std::string, std::size_t>(1024)));
}

TEST(LRUCache, try_get) {
    constexpr std::size_t TEST_CASE{10};
    LRUCache<std::string, std::size_t> cache{TEST_CASE};

    for (std::size_t i = 0; i < TEST_CASE; ++i) {
        cache.put(std::to_string(i), i);
    }

    for (std::size_t i = 0; i < TEST_CASE; ++i) {
        auto element = cache.try_get(std::to_string(i));
        EXPECT_TRUE(element.second);
        EXPECT_EQ(*element.first, i);
    }

    for (std::size_t i = TEST_CASE; i < TEST_CASE * 2; ++i) {
        auto element = cache.try_get(std::to_string(i));
        EXPECT_FALSE(element.second);
    }
}

TEST(LRUCache, GetWithReplacement) {
    LRUCache<std::string, std::size_t> cache{2};

    cache.put("1", 1);
    cache.put("2", 2);

    auto element1 = cache.get_or_die("1");
    auto element2 = cache.get_or_die("2");
    EXPECT_EQ(*element1, 1);
    EXPECT_EQ(*element2, 2);
    cache.put("3", 3);
    auto element3 = cache.get_or_die("3");
    EXPECT_EQ(*element3, 3);

    std::string replaced_key;

    for (size_t i = 1; i <= 2; ++i) {
        const auto key = std::to_string(i);

        if (!cache.contains(key)) {
            replaced_key = key;
        }
    }

    EXPECT_FALSE(cache.contains(replaced_key));
    EXPECT_FALSE(cache.try_get(replaced_key).second);
    EXPECT_THROW(cache.get_or_die(replaced_key), std::range_error);
    EXPECT_EQ(*element1, 1);
    EXPECT_EQ(*element2, 2);
    EXPECT_EQ(*element3, 3);
}

TEST(LRUCache, InvalidSize) {
    using test_type = LRUCache<std::string, int>;
    EXPECT_THROW(test_type cache{0}, std::invalid_argument);
}


TEST(NoPolicyCache, Add_one_element) {
    constexpr std::size_t cache_size = 1;
    turbo::fixed_sized_cache<std::string, int> cache(cache_size);

    cache.put("Hello", 1);
    ASSERT_EQ(*cache.get_or_die("Hello"), 1);
}

TEST(NoPolicyCache, Add_delete_add_one_element) {
    constexpr std::size_t cache_size = 1;
    turbo::fixed_sized_cache<std::string, int> cache(cache_size);

    cache.put("Hello", 1);
    cache.put("World", 2);
    ASSERT_THROW(cache.get_or_die("Hello"), std::range_error);
    ASSERT_EQ(*cache.get_or_die("World"), 2);
}

TEST(NoPolicyCache, Add_many_elements) {
    constexpr std::size_t cache_size = 1024;
    turbo::fixed_sized_cache<std::string, std::size_t> cache(cache_size);

    for (std::size_t i = 0; i < cache_size; ++i) {
        cache.put(std::to_string(i), i);
    }

    ASSERT_EQ(cache.size(), cache_size);

    for (std::size_t i = 0; i < cache_size; ++i) {
        ASSERT_EQ(*cache.get_or_die(std::to_string(i)), i);
    }
}

TEST(NoPolicyCache, Small_cache_many_elements) {
    constexpr std::size_t cache_size = 1;
    turbo::fixed_sized_cache<std::string, std::size_t> cache(cache_size);

    for (std::size_t i = 0; i < cache_size; ++i) {
        std::string temp_key = std::to_string(i);
        cache.put(temp_key, i);
        ASSERT_EQ(*cache.get_or_die(temp_key), i);
    }

    ASSERT_EQ(cache.size(), cache_size);
}

TEST(NoPolicyCache, Remove_Test) {
    constexpr std::size_t TEST_SIZE = 10;
    turbo::fixed_sized_cache<std::string, std::size_t> fc(TEST_SIZE);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        fc.put(std::to_string(i), i);
    }

    EXPECT_EQ(fc.size(), TEST_SIZE);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_TRUE(fc.remove(std::to_string(i)));
    }

    EXPECT_EQ(fc.size(), 0);

    for (std::size_t i = 0; i < TEST_SIZE; ++i) {
        EXPECT_FALSE(fc.remove(std::to_string(i)));
    }
}

TEST(NoPolicyCache, try_get) {
    constexpr std::size_t TEST_CASE{10};
    Cache<std::string, std::size_t> cache{TEST_CASE};

    for (std::size_t i = 0; i < TEST_CASE; ++i) {
        cache.put(std::to_string(i), i);
    }

    for (std::size_t i = 0; i < TEST_CASE; ++i) {
        auto element = cache.try_get(std::to_string(i));
        EXPECT_TRUE(element.second);
        EXPECT_EQ(*element.first, i);
    }

    for (std::size_t i = TEST_CASE; i < TEST_CASE * 2; ++i) {
        auto element = cache.try_get(std::to_string(i));
        EXPECT_FALSE(element.second);
    }
}

TEST(NoPolicyCache, GetWithReplacement) {
    Cache<std::string, std::size_t> cache{2};

    cache.put("1", 1);
    cache.put("2", 2);

    auto element1 = cache.get_or_die("1");
    auto element2 = cache.get_or_die("2");
    EXPECT_EQ(*element1, 1);
    EXPECT_EQ(*element2, 2);
    cache.put("3", 3);
    auto element3 = cache.get_or_die("3");
    EXPECT_EQ(*element3, 3);

    std::string replaced_key;

    for (size_t i = 1; i <= 2; ++i) {
        const auto key = std::to_string(i);

        if (!cache.contains(key)) {
            replaced_key = key;
        }
    }

    EXPECT_FALSE(cache.contains(replaced_key));
    EXPECT_FALSE(cache.try_get(replaced_key).second);
    EXPECT_THROW(cache.get_or_die(replaced_key), std::range_error);
    EXPECT_EQ(*element1, 1);
    EXPECT_EQ(*element2, 2);
    EXPECT_EQ(*element3, 3);
}

TEST(NoPolicyCache, InvalidSize) {
    using test_type = Cache<std::string, int>;
    EXPECT_THROW(test_type cache{0}, std::invalid_argument);
}
