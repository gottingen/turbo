// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef MELON_CONTAINER_LINKED_LIST_H_
#define MELON_CONTAINER_LINKED_LIST_H_

#include "turbo/platform/port.h"

// Simple linked_list type. (See the Q&A section to understand how this
// differs from std::list).
//
// To use, start by declaring the class which will be contained in the linked
// list, as extending link_node (this gives it next/previous pointers).
//
//   class MyNodeType : public link_node<MyNodeType> {
//     ...
//   };
//
// Next, to keep track of the list's head/tail, use a linked_list instance:
//
//   linked_list<MyNodeType> list;
//
// To add elements to the list, use any of linked_list::Append,
// link_node::insert_before, or link_node::insert_after:
//
//   link_node<MyNodeType>* n1 = ...;
//   link_node<MyNodeType>* n2 = ...;
//   link_node<MyNodeType>* n3 = ...;
//
//   list.Append(n1);
//   list.Append(n3);
//   n2->insert_before(n3);
//
// Lastly, to iterate through the linked list forwards:
//
//   for (link_node<MyNodeType>* node = list.head();
//        node != list.end();
//        node = node->next()) {
//     MyNodeType* value = node->value();
//     ...
//   }
//
// Or to iterate the linked list backwards:
//
//   for (link_node<MyNodeType>* node = list.tail();
//        node != list.end();
//        node = node->previous()) {
//     MyNodeType* value = node->value();
//     ...
//   }
//
// Questions and Answers:
//
// Q. Should I use std::list or melon::container::linked_list?
//
// A. The main reason to use melon::container::linked_list over std::list is
//    performance. If you don't care about the performance differences
//    then use an STL container, as it makes for better code readability.
//
//    Comparing the performance of melon::container::linked_list<T> to std::list<T*>:
//
//    * Erasing an element of type T* from melon::container::linked_list<T> is
//      an O(1) operation. Whereas for std::list<T*> it is O(n).
//      That is because with std::list<T*> you must obtain an
//      iterator to the T* element before you can call erase(iterator).
//
//    * Insertion operations with melon::container::linked_list<T> never require
//      heap allocations.
//
// Q. How does melon::container::linked_list implementation differ from std::list?
//
// A. Doubly-linked lists are made up of nodes that contain "next" and
//    "previous" pointers that reference other nodes in the list.
//
//    With melon::container::linked_list<T>, the type being inserted already reserves
//    space for the "next" and "previous" pointers (melon::container::link_node<T>*).
//    Whereas with std::list<T> the type can be anything, so the implementation
//    needs to glue on the "next" and "previous" pointers using
//    some internal node type.

namespace turbo {

    template<typename T>
    class link_node {
    public:
        // link_node are self-referential as default.
        link_node() : previous_(this), next_(this) {}

        link_node(link_node<T> *previous, link_node<T> *next)
                : previous_(previous), next_(next) {}

        // Insert |this| into the linked list, before |e|.
        void insert_before(link_node<T> *e) {
            this->next_ = e;
            this->previous_ = e->previous_;
            e->previous_->next_ = this;
            e->previous_ = this;
        }

        // Insert |this| as a circular linked list into the linked list, before |e|.
        void insert_before_as_list(link_node<T> *e) {
            link_node<T> *prev = this->previous_;
            prev->next_ = e;
            this->previous_ = e->previous_;
            e->previous_->next_ = this;
            e->previous_ = prev;
        }

        // Insert |this| into the linked list, after |e|.
        void insert_after(link_node<T> *e) {
            this->next_ = e->next_;
            this->previous_ = e;
            e->next_->previous_ = this;
            e->next_ = this;
        }

        // Insert |this| as a circular list into the linked list, after |e|.
        void insert_after_as_list(link_node<T> *e) {
            link_node<T> *prev = this->previous_;
            prev->next_ = e->next_;
            this->previous_ = e;
            e->next_->previous_ = prev;
            e->next_ = this;
        }

        // Remove |this| from the linked list.
        void remove_from_list() {
            this->previous_->next_ = this->next_;
            this->next_->previous_ = this->previous_;
            // next() and previous() return non-nullptr if and only this node is not in any
            // list.
            this->next_ = this;
            this->previous_ = this;
        }

        link_node<T> *previous() const {
            return previous_;
        }

        link_node<T> *next() const {
            return next_;
        }

        // Cast from the node-type to the value type.
        const T *value() const {
            return static_cast<const T *>(this);
        }

        T *value() {
            return static_cast<T *>(this);
        }

    private:
        link_node<T> *previous_;
        link_node<T> *next_;TURBO_NON_COPYABLE(link_node);
    };

    template<typename T>
    class linked_list {
    public:
        // The "root" node is self-referential, and forms the basis of a circular
        // list (root_.next() will point back to the start of the list,
        // and root_->previous() wraps around to the end of the list).
        linked_list() {}

        // append |e| to the end of the linked list.
        void append(link_node<T> *e) {
            e->insert_before(&root_);
        }

        link_node<T> *head() const {
            return root_.next();
        }

        link_node<T> *tail() const {
            return root_.previous();
        }

        const link_node<T> *end() const {
            return &root_;
        }

        bool empty() const { return head() == end(); }

    private:
        link_node<T> root_;TURBO_NON_COPYABLE(linked_list);
    };

}  // namespace turbo

#endif  // MELON_CONTAINER_LINKED_LIST_H_
