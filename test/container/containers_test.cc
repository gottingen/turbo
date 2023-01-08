

/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#include "flare/container/containers.h"
#include "testing/gtest_wrap.h"
#include "testing/test_help.h"
#include <cstddef>
#include <string>

class ContainersVectorTest : public with_tracked_allocator {
};

TEST_F(ContainersVectorTest, Empty) {
    flare::containers::vector<std::string, 4> vector(allocator);
    ASSERT_EQ(vector.size(), size_t(0));
}

TEST_F(ContainersVectorTest, WithinFixedCapIndex) {
    flare::containers::vector<std::string, 4> vector(allocator);
    vector.resize(4);
    vector[0] = "A";
    vector[1] = "B";
    vector[2] = "C";
    vector[3] = "D";

    ASSERT_EQ(vector[0], "A");
    ASSERT_EQ(vector[1], "B");
    ASSERT_EQ(vector[2], "C");
    ASSERT_EQ(vector[3], "D");
}

TEST_F(ContainersVectorTest, BeyondFixedCapIndex) {
    flare::containers::vector<std::string, 1> vector(allocator);
    vector.resize(4);
    vector[0] = "A";
    vector[1] = "B";
    vector[2] = "C";
    vector[3] = "D";

    ASSERT_EQ(vector[0], "A");
    ASSERT_EQ(vector[1], "B");
    ASSERT_EQ(vector[2], "C");
    ASSERT_EQ(vector[3], "D");
}

TEST_F(ContainersVectorTest, WithinFixedCapPushPop) {
    flare::containers::vector<std::string, 4> vector(allocator);
    vector.push_back("A");
    vector.push_back("B");
    vector.push_back("C");
    vector.push_back("D");

    ASSERT_EQ(vector.size(), size_t(4));
    ASSERT_EQ(vector.end() - vector.begin(), ptrdiff_t(4));

    ASSERT_EQ(vector.front(), "A");
    ASSERT_EQ(vector.back(), "D");
    vector.pop_back();
    ASSERT_EQ(vector.size(), size_t(3));
    ASSERT_EQ(vector.end() - vector.begin(), ptrdiff_t(3));

    ASSERT_EQ(vector.front(), "A");
    ASSERT_EQ(vector.back(), "C");
    vector.pop_back();
    ASSERT_EQ(vector.size(), size_t(2));
    ASSERT_EQ(vector.end() - vector.begin(), ptrdiff_t(2));

    ASSERT_EQ(vector.front(), "A");
    ASSERT_EQ(vector.back(), "B");
    vector.pop_back();
    ASSERT_EQ(vector.size(), size_t(1));
    ASSERT_EQ(vector.end() - vector.begin(), ptrdiff_t(1));

    ASSERT_EQ(vector.front(), "A");
    ASSERT_EQ(vector.back(), "A");
    vector.pop_back();
    ASSERT_EQ(vector.size(), size_t(0));
}

TEST_F(ContainersVectorTest, BeyondFixedCapPushPop) {
    flare::containers::vector<std::string, 2> vector(allocator);
    vector.push_back("A");
    vector.push_back("B");
    vector.push_back("C");
    vector.push_back("D");

    ASSERT_EQ(vector.size(), size_t(4));
    ASSERT_EQ(vector.end() - vector.begin(), ptrdiff_t(4));

    ASSERT_EQ(vector.front(), "A");
    ASSERT_EQ(vector.back(), "D");
    vector.pop_back();
    ASSERT_EQ(vector.size(), size_t(3));
    ASSERT_EQ(vector.end() - vector.begin(), ptrdiff_t(3));

    ASSERT_EQ(vector.front(), "A");
    ASSERT_EQ(vector.back(), "C");
    vector.pop_back();
    ASSERT_EQ(vector.size(), size_t(2));
    ASSERT_EQ(vector.end() - vector.begin(), ptrdiff_t(2));

    ASSERT_EQ(vector.front(), "A");
    ASSERT_EQ(vector.back(), "B");
    vector.pop_back();
    ASSERT_EQ(vector.size(), size_t(1));
    ASSERT_EQ(vector.end() - vector.begin(), ptrdiff_t(1));

    ASSERT_EQ(vector.front(), "A");
    ASSERT_EQ(vector.back(), "A");
    vector.pop_back();
    ASSERT_EQ(vector.size(), size_t(0));
}

TEST_F(ContainersVectorTest, CopyConstruct) {
    flare::containers::vector<std::string, 4> vectorA(allocator);

    vectorA.resize(3);
    vectorA[0] = "A";
    vectorA[1] = "B";
    vectorA[2] = "C";

    flare::containers::vector<std::string, 4> vectorB(vectorA, allocator);
    ASSERT_EQ(vectorB.size(), size_t(3));
    ASSERT_EQ(vectorB[0], "A");
    ASSERT_EQ(vectorB[1], "B");
    ASSERT_EQ(vectorB[2], "C");
}

TEST_F(ContainersVectorTest, CopyConstructDifferentBaseCapacity) {
    flare::containers::vector<std::string, 4> vectorA(allocator);

    vectorA.resize(3);
    vectorA[0] = "A";
    vectorA[1] = "B";
    vectorA[2] = "C";

    flare::containers::vector<std::string, 2> vectorB(vectorA, allocator);
    ASSERT_EQ(vectorB.size(), size_t(3));
    ASSERT_EQ(vectorB[0], "A");
    ASSERT_EQ(vectorB[1], "B");
    ASSERT_EQ(vectorB[2], "C");
}

TEST_F(ContainersVectorTest, CopyAssignment) {
    flare::containers::vector<std::string, 4> vectorA(allocator);

    vectorA.resize(3);
    vectorA[0] = "A";
    vectorA[1] = "B";
    vectorA[2] = "C";

    flare::containers::vector<std::string, 4> vectorB(allocator);
    vectorB = vectorA;
    ASSERT_EQ(vectorB.size(), size_t(3));
    ASSERT_EQ(vectorB[0], "A");
    ASSERT_EQ(vectorB[1], "B");
    ASSERT_EQ(vectorB[2], "C");
}

TEST_F(ContainersVectorTest, CopyAssignmentDifferentBaseCapacity) {
    flare::containers::vector<std::string, 4> vectorA(allocator);

    vectorA.resize(3);
    vectorA[0] = "A";
    vectorA[1] = "B";
    vectorA[2] = "C";

    flare::containers::vector<std::string, 2> vectorB(allocator);
    vectorB = vectorA;
    ASSERT_EQ(vectorB.size(), size_t(3));
    ASSERT_EQ(vectorB[0], "A");
    ASSERT_EQ(vectorB[1], "B");
    ASSERT_EQ(vectorB[2], "C");
}

TEST_F(ContainersVectorTest, MoveConstruct) {
    flare::containers::vector<std::string, 4> vectorA(allocator);

    vectorA.resize(3);
    vectorA[0] = "A";
    vectorA[1] = "B";
    vectorA[2] = "C";

    flare::containers::vector<std::string, 2> vectorB(std::move(vectorA),
                                                      allocator);
    ASSERT_EQ(vectorB.size(), size_t(3));
    ASSERT_EQ(vectorB[0], "A");
    ASSERT_EQ(vectorB[1], "B");
    ASSERT_EQ(vectorB[2], "C");
}

TEST_F(ContainersVectorTest, Copy) {
    flare::containers::vector<std::string, 4> vectorA(allocator);
    flare::containers::vector<std::string, 2> vectorB(allocator);

    vectorA.resize(3);
    vectorA[0] = "A";
    vectorA[1] = "B";
    vectorA[2] = "C";

    vectorB.resize(1);
    vectorB[0] = "Z";

    vectorB = vectorA;
    ASSERT_EQ(vectorB.size(), size_t(3));
    ASSERT_EQ(vectorB[0], "A");
    ASSERT_EQ(vectorB[1], "B");
    ASSERT_EQ(vectorB[2], "C");
}

TEST_F(ContainersVectorTest, Move) {
    flare::containers::vector<std::string, 4> vectorA(allocator);
    flare::containers::vector<std::string, 2> vectorB(allocator);

    vectorA.resize(3);
    vectorA[0] = "A";
    vectorA[1] = "B";
    vectorA[2] = "C";

    vectorB.resize(1);
    vectorB[0] = "Z";

    vectorB = std::move(vectorA);
    ASSERT_EQ(vectorA.size(), size_t(0));
    ASSERT_EQ(vectorB.size(), size_t(3));
    ASSERT_EQ(vectorB[0], "A");
    ASSERT_EQ(vectorB[1], "B");
    ASSERT_EQ(vectorB[2], "C");
}

class ContainersListTest : public with_tracked_allocator {
};

TEST_F(ContainersListTest, Empty) {
    flare::containers::list<std::string> list(allocator);
    ASSERT_EQ(list.size(), size_t(0));
}

TEST_F(ContainersListTest, EmplaceOne) {
    flare::containers::list<std::string> list(allocator);
    auto itEntry = list.emplace_front("hello world");
    ASSERT_EQ(*itEntry, "hello world");
    ASSERT_EQ(list.size(), size_t(1));
    auto it = list.begin();
    ASSERT_EQ(it, itEntry);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, EmplaceThree) {
    flare::containers::list<std::string> list(allocator);
    auto itA = list.emplace_front("a");
    auto itB = list.emplace_front("b");
    auto itC = list.emplace_front("c");
    ASSERT_EQ(*itA, "a");
    ASSERT_EQ(*itB, "b");
    ASSERT_EQ(*itC, "c");
    ASSERT_EQ(list.size(), size_t(3));
    auto it = list.begin();
    ASSERT_EQ(it, itC);
    ++it;
    ASSERT_EQ(it, itB);
    ++it;
    ASSERT_EQ(it, itA);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, EraseFront) {
    flare::containers::list<std::string> list(allocator);
    auto itA = list.emplace_front("a");
    auto itB = list.emplace_front("b");
    auto itC = list.emplace_front("c");
    list.erase(itC);
    ASSERT_EQ(list.size(), size_t(2));
    auto it = list.begin();
    ASSERT_EQ(it, itB);
    ++it;
    ASSERT_EQ(it, itA);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, EraseBack) {
    flare::containers::list<std::string> list(allocator);
    auto itA = list.emplace_front("a");
    auto itB = list.emplace_front("b");
    auto itC = list.emplace_front("c");
    list.erase(itA);
    ASSERT_EQ(list.size(), size_t(2));
    auto it = list.begin();
    ASSERT_EQ(it, itC);
    ++it;
    ASSERT_EQ(it, itB);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, EraseMid) {
    flare::containers::list<std::string> list(allocator);
    auto itA = list.emplace_front("a");
    auto itB = list.emplace_front("b");
    auto itC = list.emplace_front("c");
    list.erase(itB);
    ASSERT_EQ(list.size(), size_t(2));
    auto it = list.begin();
    ASSERT_EQ(it, itC);
    ++it;
    ASSERT_EQ(it, itA);
    ++it;
    ASSERT_EQ(it, list.end());
}

TEST_F(ContainersListTest, Grow) {
    flare::containers::list<std::string> list(allocator);
    for (int i = 0; i < 256; i++) {
        list.emplace_front(std::to_string(i));
    }
    ASSERT_EQ(list.size(), size_t(256));
}
