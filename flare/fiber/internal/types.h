// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// fiber - A M:N threading library to make applications more concurrent.

// Date: Tue Jul 10 17:40:58 CST 2012

#ifndef FLARE_FIBER_INTERNAL__TYPES_H_
#define FLARE_FIBER_INTERNAL__TYPES_H_

#include <stdint.h>                            // uint64_t

#if defined(__cplusplus)

#include "flare/log/logging.h"                      // FLARE_CHECK

#endif

typedef uint64_t fiber_id_t;

// tid returned by fiber_start_* never equals this value.
static const fiber_id_t INVALID_FIBER_ID = 0;

struct sockaddr;

typedef unsigned fiber_stack_type_t;
static const fiber_stack_type_t FIBER_STACKTYPE_UNKNOWN = 0;
static const fiber_stack_type_t FIBER_STACKTYPE_PTHREAD = 1;
static const fiber_stack_type_t FIBER_STACKTYPE_SMALL = 2;
static const fiber_stack_type_t FIBER_STACKTYPE_NORMAL = 3;
static const fiber_stack_type_t FIBER_STACKTYPE_LARGE = 4;

typedef unsigned fiber_attribute_flag;
static const fiber_attribute_flag FIBER_LOG_START_AND_FINISH = 8;
static const fiber_attribute_flag FIBER_LOG_CONTEXT_SWITCH = 16;
static const fiber_attribute_flag FIBER_NOSIGNAL = 32;
static const fiber_attribute_flag FIBER_NEVER_QUIT = 64;

// Key of thread-local data, created by fiber_key_create.
typedef struct {
    uint32_t index;    // index in KeyTable
    uint32_t version;  // ABA avoidance
} fiber_local_key;

static const fiber_local_key INVALID_FIBER_KEY = {0, 0};

#if defined(__cplusplus)

// Overload operators for fiber_local_key
inline bool operator==(fiber_local_key key1, fiber_local_key key2) {
    return key1.index == key2.index && key1.version == key2.version;
}

inline bool operator!=(fiber_local_key key1, fiber_local_key key2) { return !(key1 == key2); }

inline bool operator<(fiber_local_key key1, fiber_local_key key2) {
    return key1.index != key2.index ? (key1.index < key2.index) :
           (key1.version < key2.version);
}

inline bool operator>(fiber_local_key key1, fiber_local_key key2) { return key2 < key1; }

inline bool operator<=(fiber_local_key key1, fiber_local_key key2) { return !(key2 < key1); }

inline bool operator>=(fiber_local_key key1, fiber_local_key key2) { return !(key1 < key2); }

inline std::ostream &operator<<(std::ostream &os, fiber_local_key key) {
    return os << "fiber_local_key{index=" << key.index << " version="
              << key.version << '}';
}

#endif  // __cplusplus

typedef struct {
    pthread_mutex_t mutex;
    void *free_keytables;
    int destroyed;
} fiber_keytable_pool_t;

typedef struct {
    size_t nfree;
} fiber_keytable_pool_stat_t;

// Attributes for thread creation.
typedef struct fiber_attribute {
    fiber_stack_type_t stack_type;
    fiber_attribute_flag flags;
    fiber_keytable_pool_t *keytable_pool;

#if defined(__cplusplus)

    void operator=(unsigned stacktype_and_flags) {
        stack_type = (stacktype_and_flags & 7);
        flags = (stacktype_and_flags & ~(unsigned) 7u);
        keytable_pool = nullptr;
    }

    fiber_attribute operator|(unsigned other_flags) const {
        FLARE_CHECK(!(other_flags & 7)) << "flags=" << other_flags;
        fiber_attribute tmp = *this;
        tmp.flags |= (other_flags & ~(unsigned) 7u);
        return tmp;
    }

#endif  // __cplusplus
} fiber_attribute;

// fibers started with this attribute will run on stack of worker pthread and
// all fiber functions that would block the fiber will block the pthread.
// The fiber will not allocate its own stack, simply occupying a little meta
// memory. This is required to run JNI code which checks layout of stack. The
// obvious drawback is that you need more worker pthreads when you have a lot
// of such fibers.
static const fiber_attribute FIBER_ATTR_PTHREAD =
        {FIBER_STACKTYPE_PTHREAD, 0, nullptr};

// fibers created with following attributes will have different size of
// stacks. Default is FIBER_ATTR_NORMAL.
static const fiber_attribute FIBER_ATTR_SMALL =
        {FIBER_STACKTYPE_SMALL, 0, nullptr};
static const fiber_attribute FIBER_ATTR_NORMAL =
        {FIBER_STACKTYPE_NORMAL, 0, nullptr};
static const fiber_attribute FIBER_ATTR_LARGE =
        {FIBER_STACKTYPE_LARGE, 0, nullptr};

// fibers created with this attribute will print log when it's started,
// context-switched, finished.
static const fiber_attribute FIBER_ATTR_DEBUG = {
        FIBER_STACKTYPE_NORMAL,
        FIBER_LOG_START_AND_FINISH | FIBER_LOG_CONTEXT_SWITCH,
        nullptr
};

static const size_t FIBER_EPOLL_THREAD_NUM = 1;
static const fiber_id_t FIBER_ATOMIC_INIT = 0;

// Min/Max number of work pthreads.
static const int FIBER_MIN_CONCURRENCY = 3 + FIBER_EPOLL_THREAD_NUM;
static const int FIBER_MAX_CONCURRENCY = 1024;

typedef struct {
    void *impl;
    // following fields are part of previous impl. and not used right now.
    // Don't remove them to break ABI compatibility.
    unsigned head;
    unsigned size;
    unsigned conflict_head;
    unsigned conflict_size;
} fiber_list_t;

typedef struct {
    int64_t duration_ns;
    size_t sampling_range;
} fiber_contention_site_t;

typedef struct {
    unsigned *event;
    fiber_contention_site_t csite;
} fiber_mutex_t;

typedef struct {
} fiber_mutexattr_t;

typedef struct {
    fiber_mutex_t *m;
    int *seq;
} fiber_cond_t;

typedef struct {
} fiber_condattr_t;

typedef struct {
} fiber_rwlock_t;

typedef struct {
} fiber_rwlockattr_t;

typedef struct {
    unsigned int count;
} fiber_barrier_t;

typedef struct {
} fiber_barrierattr_t;

typedef struct {
    uint64_t value;
} fiber_token_t;

// fiber_token returned by fiber_token_create* can never be this value.
// NOTE: don't confuse with INVALID_FIBER_ID!
static const fiber_token_t INVALID_FIBER_TOKEN = {0};

#if defined(__cplusplus)

// Overload operators for fiber_token_t
inline bool operator==(fiber_token_t id1, fiber_token_t id2) { return id1.value == id2.value; }

inline bool operator!=(fiber_token_t id1, fiber_token_t id2) { return !(id1 == id2); }

inline bool operator<(fiber_token_t id1, fiber_token_t id2) { return id1.value < id2.value; }

inline bool operator>(fiber_token_t id1, fiber_token_t id2) { return id2 < id1; }

inline bool operator<=(fiber_token_t id1, fiber_token_t id2) { return !(id2 < id1); }

inline bool operator>=(fiber_token_t id1, fiber_token_t id2) { return !(id1 < id2); }

inline std::ostream &operator<<(std::ostream &os, fiber_token_t id) { return os << id.value; }

#endif  // __cplusplus

typedef struct {
    void *impl;
    // following fields are part of previous impl. and not used right now.
    // Don't remove them to break ABI compatibility.
    unsigned head;
    unsigned size;
    unsigned conflict_head;
    unsigned conflict_size;
} fiber_token_list_t;

typedef uint64_t fiber_timer_id;

#endif  // FLARE_FIBER_INTERNAL__TYPES_H_
