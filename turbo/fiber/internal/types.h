// Copyright 2023 The Elastic-AI Authors.
// part of Elastic AI Search
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
//
//
// Created by jeff on 24-1-3.
//

#ifndef TURBO_FIBER_INTERNAL_TYPES_H_
#define TURBO_FIBER_INTERNAL_TYPES_H_

#include <cstdint>
#include "turbo/log/logging.h"

namespace turbo::fiber_internal {
    typedef uint64_t fiber_id_t;

    typedef std::function<void *(void *)> fiber_fn_t;

    // tid returned by fiber_start_* never equals this value.
    static const fiber_id_t INVALID_FIBER_ID = 0;

    struct sockaddr;


    enum class StackType : unsigned {
        STACK_TYPE_MAIN = 0,
        STACK_TYPE_PTHREAD = 1,
        STACK_TYPE_SMALL = 2,
        STACK_TYPE_NORMAL = 3,
        STACK_TYPE_LARGE = 4
    };

    enum class AttributeFlag : unsigned {
        FLAG_NONE = 0,
        FLAG_LOG_START_AND_FINISH = 8,
        FLAG_LOG_CONTEXT_SWITCH = 16,
        FLAG_NOSIGNAL = 32,
        FLAG_NEVER_QUIT = 64
    };

    inline constexpr AttributeFlag operator|(AttributeFlag lhs, AttributeFlag rhs) {
        return static_cast<AttributeFlag>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
    }

    inline constexpr AttributeFlag operator&(AttributeFlag lhs, AttributeFlag rhs) {
        return static_cast<AttributeFlag>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
    }

    inline constexpr AttributeFlag operator~(AttributeFlag rhs) {
        return static_cast<AttributeFlag>(~static_cast<unsigned>(rhs));
    }

    inline constexpr AttributeFlag &operator|=(AttributeFlag &lhs, AttributeFlag rhs) {
        lhs = lhs | rhs;
        return lhs;
    }


    inline constexpr AttributeFlag &operator&=(AttributeFlag &lhs, AttributeFlag rhs) {
        lhs = lhs & rhs;
        return lhs;
    }


    typedef struct {
        std::mutex mutex;
        void *free_keytables;
        int destroyed;
    } fiber_keytable_pool_t;

    typedef struct {
        size_t nfree;
    } fiber_keytable_pool_stat_t;

    // Attributes for thread creation.
    struct FiberAttribute {
        StackType stack_type;
        AttributeFlag flags;
        fiber_keytable_pool_t *keytable_pool;

        void operator=(unsigned stacktype_and_flags) {
            const unsigned stack_type_int = (stacktype_and_flags & 7);
            const unsigned flag_int = (stacktype_and_flags & ~(unsigned) 7u);
            stack_type = static_cast<StackType>(stack_type_int);
            flags = static_cast<AttributeFlag>(flag_int);
            keytable_pool = nullptr;
        }

        FiberAttribute operator|(AttributeFlag other_flags) const {
            FiberAttribute tmp = *this;
            tmp.flags |= other_flags;
            return tmp;
        }

    };

    inline constexpr bool is_pthread_stack(const FiberAttribute &attr) {
        return attr.stack_type == StackType::STACK_TYPE_PTHREAD;
    }

    inline constexpr bool is_small_stack(const FiberAttribute &attr) {
        return attr.stack_type == StackType::STACK_TYPE_SMALL;
    }

    inline constexpr bool is_normal_stack(const FiberAttribute &attr) {
        return attr.stack_type == StackType::STACK_TYPE_NORMAL;
    }

    inline constexpr bool is_large_stack(const FiberAttribute &attr) {
        return attr.stack_type == StackType::STACK_TYPE_LARGE;
    }

    inline constexpr bool is_main_stack(const FiberAttribute &attr) {
        return attr.stack_type == StackType::STACK_TYPE_MAIN;
    }

    inline constexpr bool is_never_quit(const FiberAttribute &attr) {
        return (attr.flags & AttributeFlag::FLAG_NEVER_QUIT) != AttributeFlag::FLAG_NONE;
    }

    inline constexpr bool is_nosignal(const FiberAttribute &attr) {
        return (attr.flags & AttributeFlag::FLAG_NOSIGNAL) != AttributeFlag::FLAG_NONE;
    }

    inline constexpr bool is_log_start_and_finish(const FiberAttribute &attr) {
        return (attr.flags & AttributeFlag::FLAG_LOG_START_AND_FINISH) != AttributeFlag::FLAG_NONE;
    }

    inline constexpr bool is_log_context_switch(const FiberAttribute &attr) {
        return (attr.flags & AttributeFlag::FLAG_LOG_CONTEXT_SWITCH) != AttributeFlag::FLAG_NONE;
    }

    // fibers started with this attribute will run on stack of worker pthread and
    // all fiber functions that would block the fiber will block the pthread.
    // The fiber will not allocate its own stack, simply occupying a little meta
    // memory. This is required to run JNI code which checks layout of stack. The
    // obvious drawback is that you need more worker pthreads when you have a lot
    // of such fibers.
    static constexpr FiberAttribute FIBER_ATTR_PTHREAD =
            {StackType::STACK_TYPE_PTHREAD, AttributeFlag::FLAG_NONE, nullptr};

    // fibers created with following attributes will have different size of
    // stacks. Default is FIBER_ATTR_NORMAL.
    static constexpr FiberAttribute FIBER_ATTR_SMALL =
            {StackType::STACK_TYPE_SMALL, AttributeFlag::FLAG_NONE, nullptr};
    static constexpr FiberAttribute FIBER_ATTR_NORMAL =
            {StackType::STACK_TYPE_NORMAL, AttributeFlag::FLAG_NONE, nullptr};
    static constexpr FiberAttribute FIBER_ATTR_LARGE =
            {StackType::STACK_TYPE_LARGE, AttributeFlag::FLAG_NONE, nullptr};

    static constexpr FiberAttribute FIBER_ATTR_MAIN = {
            StackType::STACK_TYPE_MAIN, AttributeFlag::FLAG_NONE, nullptr};

    // fibers created with this attribute will print log when it's started,
    // context-switched, finished.
    static constexpr FiberAttribute FIBER_ATTR_DEBUG = {
            StackType::STACK_TYPE_NORMAL,
            AttributeFlag::FLAG_LOG_START_AND_FINISH | AttributeFlag::FLAG_LOG_CONTEXT_SWITCH,
            nullptr
    };


    typedef struct {
        void *impl;
        // following fields are part of previous impl. and not used right now.
        // Don't remove them to break ABI compatibility.
        unsigned head;
        unsigned size;
        unsigned conflict_head;
        unsigned conflict_size;
    } fiber_list_t;

    // fiber_token returned by fiber_session_create* can never be this value.
    // NOTE: don't confuse with INVALID_FIBER_ID!
    static constexpr uint64_t INVALID_FIBER_SESSION_VALUE = 0;

    struct FiberSessionImpl {
        uint64_t value{INVALID_FIBER_SESSION_VALUE};
    };

    static constexpr FiberSessionImpl INVALID_FIBER_SESSION = {0};

    typedef std::function<int(FiberSessionImpl session,void *data, int error_code) > session_on_error;

    typedef std::function<int(FiberSessionImpl session,void *data, int error_code, const std::string &error_text) > session_on_error_msg;

    // Overload operators for FiberSessionImpl
    inline bool operator==(FiberSessionImpl id1, FiberSessionImpl id2) { return id1.value == id2.value; }

    inline bool operator!=(FiberSessionImpl id1, FiberSessionImpl id2) { return !(id1 == id2); }

    inline bool operator<(FiberSessionImpl id1, FiberSessionImpl id2) { return id1.value < id2.value; }

    inline bool operator>(FiberSessionImpl id1, FiberSessionImpl id2) { return id2 < id1; }

    inline bool operator<=(FiberSessionImpl id1, FiberSessionImpl id2) { return !(id2 < id1); }

    inline bool operator>=(FiberSessionImpl id1, FiberSessionImpl id2) { return !(id1 < id2); }

    inline std::ostream &operator<<(std::ostream &os, FiberSessionImpl id) { return os << id.value; }


    typedef struct {
        void *impl;
        // following fields are part of previous impl. and not used right now.
        // Don't remove them to break ABI compatibility.
        unsigned head;
        unsigned size;
        unsigned conflict_head;
        unsigned conflict_size;
    } FiberSessionList;

    typedef uint64_t fiber_timer_id;

}  // namespace turbo::fiber_internal
#endif  // TURBO_FIBER_INTERNAL_TYPES_H_
