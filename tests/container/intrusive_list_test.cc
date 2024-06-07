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

#include <gtest/gtest.h>
#include <cstddef>
#include <stdarg.h>
#include "turbo/container/intrusive_list.h"

namespace {
    template<typename InputIterator1, typename InputIterator2>
    bool check_sequence_eq(InputIterator1 firstActual, InputIterator1 lastActual, InputIterator2 firstExpected,
                           InputIterator2 lastExpected, const char *pName) {
        size_t numMatching = 0;

        while ((firstActual != lastActual) && (firstExpected != lastExpected) && (*firstActual == *firstExpected)) {
            ++firstActual;
            ++firstExpected;
            ++numMatching;
        }

        if (firstActual == lastActual && firstExpected == lastExpected) {
            return true;
        } else if (firstActual != lastActual && firstExpected == lastExpected) {
            size_t numActual = numMatching, numExpected = numMatching;
            for (; firstActual != lastActual; ++firstActual)
                ++numActual;
            if (pName)
                printf("[%s] Too many elements: expected %u, found %u\n", pName, numExpected, numActual);
            else
                printf("Too many elements: expected %u, found %u\n", numExpected, numActual);
            return false;
        } else if (firstActual == lastActual && firstExpected != lastExpected) {
            size_t numActual = numMatching, numExpected = numMatching;
            for (; firstExpected != lastExpected; ++firstExpected)
                ++numExpected;
            if (pName)
                printf("[%s] Too few elements: expected %u, found %u\n", pName, numExpected, numActual);
            else
                printf("Too few elements: expected %u, found %u\n", numExpected, numActual);
            return false;
        } else // if (firstActual != lastActual && firstExpected != lastExpected)
        {
            if (pName)
                printf("[%s] Mismatch at index %u\n", pName, numMatching);
            else
                printf("Mismatch at index %u\n", numMatching);
            return false;
        }
    }

    template<typename InputIterator, typename T = typename InputIterator::value_type>
    bool check_sequence_eq(InputIterator firstActual, InputIterator lastActual, std::initializer_list<T> initList,
                           const char *pName) {
        return check_sequence_eq(firstActual, lastActual, initList.begin(), initList.end(), pName);
    }

    template<typename Container, typename T = typename Container::value_type>
    bool check_sequence_eq(const Container &container, std::initializer_list<T> initList, const char *pName) {
        return check_sequence_eq(container.begin(), container.end(), initList.begin(), initList.end(), pName);
    }


    /// check_sequence_eq
    ///
    /// Allows the user to specify that a container has a given set of values.
    ///
    /// Example usage:
    ///    vector<int> v;
    ///    v.push_back(1); v.push_back(3); v.push_back(5);
    ///    check_sequence_eq(v.begin(), v.end(), int(), "v.push_back", 1, 3, 5, -1);
    ///
    /// Note: The StackValue template argument is a hint to the compiler about what type
    ///       the passed vararg sequence is.
    ///
    template<typename InputIterator, typename StackValue>
    bool check_sequence_eq(InputIterator first, InputIterator last, StackValue /*unused*/, const char *pName, ...) {
        typedef typename std::iterator_traits<InputIterator>::value_type value_type;

        int argIndex = 0;
        int seqIndex = 0;
        bool bReturnValue = true;
        StackValue next;

        va_list args;
        va_start(args, pName);

        for (; first != last; ++first, ++argIndex, ++seqIndex) {
            next = va_arg(args, StackValue);

            if ((next == StackValue(-1)) || !(value_type(next) == *first)) {
                if (pName)
                    printf("[%s] Mismatch at index %d\n", pName, argIndex);
                else
                    printf("Mismatch at index %d\n", argIndex);
                bReturnValue = false;
            }
        }

        for (; first != last; ++first)
            ++seqIndex;

        if (bReturnValue) {
            next = va_arg(args, StackValue);

            if (!(next == StackValue(-1))) {
                do {
                    ++argIndex;
                    next = va_arg(args, StackValue);
                } while (!(next == StackValue(-1)));

                if (pName)
                    printf("[%s] Too many elements: expected %d, found %d\n", pName, argIndex, seqIndex);
                else
                    printf("Too many elements: expected %d, found %d\n", argIndex, seqIndex);
                bReturnValue = false;
            }
        }

        va_end(args);

        return bReturnValue;
    }

    using namespace turbo;

    /// IntNode
    ///
    /// Test intrusive_list node.
    ///
    struct IntNode : public turbo::intrusive_list_node {
        int mX;

        IntNode(int x = 0)
                : mX(x) {}

        operator int() const { return mX; }
    };


    /// ListInit
    ///
    /// Utility class for setting up a list.
    ///
    class ListInit {
    public:
        ListInit(intrusive_list<IntNode> &container, IntNode *pNodeArray)
                : mpContainer(&container), mpNodeArray(pNodeArray) {
            mpContainer->clear();
        }

        ListInit &operator+=(int x) {
            mpNodeArray->mX = x;
            mpContainer->push_back(*mpNodeArray++);
            return *this;
        }

        ListInit &operator,(int x) {
            mpNodeArray->mX = x;
            mpContainer->push_back(*mpNodeArray++);
            return *this;
        }

    protected:
        intrusive_list<IntNode> *mpContainer;
        IntNode *mpNodeArray;
    };

} // namespace




// Template instantations.
// These tell the compiler to compile all the functions for the given class.
template
class turbo::intrusive_list<IntNode>;


TEST(intrusive_list, list) {

    int i;
    {
        IntNode nodes[20];

        intrusive_list<IntNode> ilist;

#ifndef __GNUC__ // GCC warns on this, though strictly specaking it is allowed to.
        // Enforce that offsetof() can be used with an intrusive_list in a struct;
            // it requires a POD type. Some compilers will flag warnings or even errors
            // when this is violated.
            struct Test {
                intrusive_list<IntNode> m;
            };
            (void)offsetof(Test, m);
#endif

        // begin / end
        (check_sequence_eq(ilist.begin(), ilist.end(), int(), "ctor()", -1));


        // push_back
        ListInit(ilist, nodes) += 0, 1, 2, 3, 4, 5, 6, 7, 8, 9;
        ASSERT_TRUE(
                check_sequence_eq(ilist.begin(), ilist.end(), int(), "push_back()", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1));


        // iterator / begin
        intrusive_list<IntNode>::iterator it = ilist.begin();
        ASSERT_TRUE(it->mX == 0);
        ++it;
        ASSERT_TRUE(it->mX == 1);
        ++it;
        ASSERT_TRUE(it->mX == 2);
        ++it;
        ASSERT_TRUE(it->mX == 3);


        // const_iterator / begin
        const intrusive_list<IntNode> cilist;
        intrusive_list<IntNode>::const_iterator cit;
        for (cit = cilist.begin(); cit != cilist.end(); ++cit)
            ASSERT_TRUE(cit == cilist.end()); // This is guaranteed to be false.


        // reverse_iterator / rbegin
        intrusive_list<IntNode>::reverse_iterator itr = ilist.rbegin();
        ASSERT_TRUE(itr->mX == 9);
        ++itr;
        ASSERT_TRUE(itr->mX == 8);
        ++itr;
        ASSERT_TRUE(itr->mX == 7);
        ++itr;
        ASSERT_TRUE(itr->mX == 6);


        // iterator++/--
        {
            intrusive_list<IntNode>::iterator it1(ilist.begin());
            intrusive_list<IntNode>::iterator it2(ilist.begin());

            ++it1;
            ++it2;
            if ((it1 != it2++) || (++it1 != it2)) {
                ASSERT_TRUE(!"[iterator::increment] fail\n");
            }

            if ((it1 != it2--) || (--it1 != it2)) {
                ASSERT_TRUE(!"[iterator::decrement] fail\n");
            }
        }


        // clear / empty
        ASSERT_TRUE(!ilist.empty());

        ilist.clear();
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "clear()", -1));
        ASSERT_TRUE(ilist.empty());


        // splice
        ListInit(ilist, nodes) += 0, 1, 2, 3, 4, 5, 6, 7, 8, 9;

        ilist.splice(++ilist.begin(), ilist, --ilist.end());
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "splice(single)", 0, 9, 1, 2, 3, 4, 5, 6, 7, 8,
                                      -1));

        intrusive_list<IntNode> ilist2;
        ListInit(ilist2, nodes + 10) += 10, 11, 12, 13, 14, 15, 16, 17, 18, 19;

        ilist.splice(++ ++ilist.begin(), ilist2);
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "splice(whole)", -1));
        ASSERT_TRUE(
                check_sequence_eq(ilist.begin(), ilist.end(), int(), "splice(whole)", 0, 9, 10, 11, 12, 13, 14, 15, 16,
                                  17,
                                  18, 19, 1, 2, 3, 4, 5, 6, 7, 8, -1));

        ilist.splice(ilist.begin(), ilist, ++ ++ilist.begin(), -- --ilist.end());
        ASSERT_TRUE(
                check_sequence_eq(ilist.begin(), ilist.end(), int(), "splice(range)", 10, 11, 12, 13, 14, 15, 16, 17,
                                  18,
                                  19, 1, 2, 3, 4, 5, 6, 0, 9, 7, 8, -1));

        ilist.clear();
        ilist.swap(ilist2);
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "swap(empty)", -1));
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "swap(empty)", -1));

        ilist2.push_back(nodes[0]);
        ilist.splice(ilist.begin(), ilist2);
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "splice(single)", 0, -1));
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "splice(single)", -1));


        // splice(single) -- evil case (splice at or right after current position)
        ListInit(ilist, nodes) += 0, 1, 2, 3, 4;
        ilist.splice(++ ++ilist.begin(), *++ ++ilist.begin());
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "splice(single)", 0, 1, 2, 3, 4, -1));
        ilist.splice(++ ++ ++ilist.begin(), *++ ++ilist.begin());
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "splice(single)", 0, 1, 2, 3, 4, -1));


        // splice(range) -- evil case (splice right after current position)
        ListInit(ilist, nodes) += 0, 1, 2, 3, 4;
        ilist.splice(++ ++ilist.begin(), ilist, ++ilist.begin(), ++ ++ilist.begin());
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "splice(range)", 0, 1, 2, 3, 4, -1));


        // push_front / push_back
        ilist.clear();
        ilist2.clear();
        for (i = 4; i >= 0; --i)
            ilist.push_front(nodes[i]);
        for (i = 5; i < 10; ++i)
            ilist2.push_back(nodes[i]);

        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "push_front()", 0, 1, 2, 3, 4, -1));
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "push_back()", 5, 6, 7, 8, 9, -1));

        for (i = 4; i >= 0; --i) {
            ilist.pop_front();
            ilist2.pop_back();
        }
        auto em = ilist2.empty() && ilist.empty();
        ASSERT_TRUE(ilist.empty());
        ASSERT_TRUE(ilist2.empty());
        ASSERT_TRUE(em);
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "pop_front()", -1));
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "pop_back()", -1));


        // contains / locate
        for (i = 0; i < 5; ++i)
            ilist.push_back(nodes[i]);

        ASSERT_TRUE(ilist.contains(nodes[2]));
        ASSERT_TRUE(!ilist.contains(nodes[7]));

        it = ilist.locate(nodes[3]);
        ASSERT_TRUE(it->mX == 3);

        it = ilist.locate(nodes[8]);
        ASSERT_TRUE(it == ilist.end());


        // reverse
        ilist.reverse();
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "push_front()", 4, 3, 2, 1, 0, -1));


        // swap()
        ilist.swap(ilist2);
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "swap()", -1));
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "swap()", 4, 3, 2, 1, 0, -1));


        // erase()
        ListInit(ilist2, nodes) += 0, 1, 2, 3, 4;
        ListInit(ilist, nodes + 5) += 5, 6, 7, 8, 9;
        ilist.erase(++ ++ilist.begin());
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "erase(single)", 5, 6, 8, 9, -1));

        ilist.erase(ilist.begin(), ilist.end());
        ASSERT_TRUE(check_sequence_eq(ilist.begin(), ilist.end(), int(), "erase(all)", -1));

        ilist2.erase(++ilist2.begin(), -- --ilist2.end());
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "erase(range)", 0, 3, 4, -1));


        // size
        ASSERT_TRUE(ilist2.size() == 3);


        // pop_front / pop_back
        ilist2.pop_front();
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "pop_front()", 3, 4, -1));

        ilist2.pop_back();
        ASSERT_TRUE(check_sequence_eq(ilist2.begin(), ilist2.end(), int(), "pop_back()", 3, -1));
    }


    {
        // void sort()
        // void sort(Compare compare)

        const int kSize = 10;
        IntNode nodes[kSize];

        intrusive_list<IntNode> listEmpty;
        listEmpty.sort();
        ASSERT_TRUE(check_sequence_eq(listEmpty.begin(), listEmpty.end(), int(), "list::sort", -1));

        intrusive_list<IntNode> list1;
        ListInit(list1, nodes) += 1;
        list1.sort();
        ASSERT_TRUE(check_sequence_eq(list1.begin(), list1.end(), int(), "list::sort", 1, -1));
        list1.clear();

        intrusive_list<IntNode> list4;
        ListInit(list4, nodes) += 1, 9, 2, 3;
        list4.sort();
        ASSERT_TRUE(check_sequence_eq(list4.begin(), list4.end(), int(), "list::sort", 1, 2, 3, 9, -1));
        list4.clear();

        intrusive_list<IntNode> listA;
        ListInit(listA, nodes) += 1, 9, 2, 3, 5, 7, 4, 6, 8, 0;
        listA.sort();
        ASSERT_TRUE(
                check_sequence_eq(listA.begin(), listA.end(), int(), "list::sort", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1));
        listA.clear();

        intrusive_list<IntNode> listB;
        ListInit(listB, nodes) += 1, 9, 2, 3, 5, 7, 4, 6, 8, 0;
        listB.sort(std::less<int>());
        ASSERT_TRUE(
                check_sequence_eq(listB.begin(), listB.end(), int(), "list::sort", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1));
        listB.clear();
    }


    {
        // void merge(this_type& x);
        // void merge(this_type& x, Compare compare);

        const int kSize = 8;
        IntNode nodesA[kSize];
        IntNode nodesB[kSize];

        intrusive_list<IntNode> listA;
        ListInit(listA, nodesA) += 1, 2, 3, 4, 4, 5, 9, 9;

        intrusive_list<IntNode> listB;
        ListInit(listB, nodesB) += 1, 2, 3, 4, 4, 5, 9, 9;

        listA.merge(listB);
        ASSERT_TRUE(
                check_sequence_eq(listA.begin(), listA.end(), int(), "list::merge", 1, 1, 2, 2, 3, 3, 4, 4, 4, 4, 5, 5,
                                  9,
                                  9, 9, 9, -1));
        ASSERT_TRUE(check_sequence_eq(listB.begin(), listB.end(), int(), "list::merge", -1));
    }


    {
        // void unique();
        // void unique(BinaryPredicate);

        const int kSize = 8;
        IntNode nodesA[kSize];
        IntNode nodesB[kSize];

        intrusive_list<IntNode> listA;
        ListInit(listA, nodesA) += 1, 2, 3, 4, 4, 5, 9, 9;
        listA.unique();
        ASSERT_TRUE(check_sequence_eq(listA.begin(), listA.end(), int(), "list::unique", 1, 2, 3, 4, 5, 9, -1));

        intrusive_list<IntNode> listB;
        ListInit(listB, nodesB) += 1, 2, 3, 4, 4, 5, 9, 9;
        listB.unique(std::equal_to<int>());
        ASSERT_TRUE(check_sequence_eq(listA.begin(), listA.end(), int(), "list::unique", 1, 2, 3, 4, 5, 9, -1));
    }


}












