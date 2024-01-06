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
// Created by jeff on 23-12-16.
//


#ifndef TURBO_FIBER_INTERNAL_STACK_H_
#define TURBO_FIBER_INTERNAL_STACK_H_

#include <assert.h>
#include "turbo/fiber/internal/types.h"
#include "turbo/fiber/internal/context.h"
#include "turbo/fiber/config.h"
#include "turbo/memory/object_pool.h"

namespace turbo::fiber_internal {

    struct StackStorage {
        int stacksize{0};
        int guardsize{0};
        // Assume stack grows upwards.
        // http://www.boost.org/doc/libs/1_55_0/libs/context/doc/html/context/stack.html
        void *bottom{nullptr};

        // Clears all members.
        void zeroize() {
            stacksize = 0;
            guardsize = 0;
            bottom = nullptr;
        }
    };

    // Allocate a piece of stack.
    int allocate_stack_storage(StackStorage *s, int stacksize, int guardsize);

    // Deallocate a piece of stack. Parameters MUST be returned or set by the
    // corresponding allocate_stack_storage() otherwise behavior is undefined.
    void deallocate_stack_storage(StackStorage *s);

    struct ContextualStack {
        fiber_fcontext_t context;
        StackType stacktype;
        StackStorage storage;
    };

    // Get a stack in the `type' and run `entry' at the first time that the
    // stack is jumped.
    ContextualStack *get_stack(StackType type, void (*entry)(intptr_t));

    // Recycle a stack. nullptr does nothing.
    void return_stack(ContextualStack *);

    // Jump from stack `from' to stack `to'. `from' must be the stack of callsite
    // (to save contexts before jumping)
    void jump_stack(ContextualStack *from, ContextualStack *to);


    struct MainStackClass {
    };

    struct SmallStackClass {
        static constexpr int stack_size_flag = FiberStackConfig::stack_size_small;
        // Older gcc does not allow static const enum, use int instead.
        static constexpr StackType stack_type = StackType::STACK_TYPE_SMALL;
    };

    struct NormalStackClass {
        static constexpr int stack_size_flag = FiberStackConfig::stack_size_normal;
        static const StackType stack_type = StackType::STACK_TYPE_NORMAL;
    };

    struct LargeStackClass {
        static constexpr int stack_size_flag = FiberStackConfig::stack_size_large;
        static const StackType stack_type = StackType::STACK_TYPE_LARGE;
    };

    template<typename StackClass>
    struct StackFactory {
        struct Wrapper : public ContextualStack {
            Wrapper() = default;
            explicit Wrapper(void (*entry)(intptr_t)) {
                if (allocate_stack_storage(&storage, StackClass::stack_size_flag,
                                           FiberStackConfig::guard_page_size) != 0) {
                    storage.zeroize();
                    context = nullptr;
                    return;
                }
                context = ::fiber_make_fcontext(storage.bottom, storage.stacksize, entry);
                stacktype = (StackType) StackClass::stack_type;
            }

            ~Wrapper() {
                if (context) {
                    context = nullptr;
                    deallocate_stack_storage(&storage);
                    storage.zeroize();
                }
            }
        };

        static ContextualStack *get_stack(void (*entry)(intptr_t)) {
            return turbo::get_object<Wrapper>(entry);
        }

        static void return_stack(ContextualStack *sc) {
            turbo::return_object(static_cast<Wrapper *>(sc));
        }
    };

    template<>
    struct StackFactory<MainStackClass> {
        static ContextualStack *get_stack(void (*)(intptr_t)) {
            ContextualStack *s = new(std::nothrow) ContextualStack;
            if (nullptr == s) {
                return nullptr;
            }
            s->context = nullptr;
            s->stacktype = StackType::STACK_TYPE_MAIN;
            s->storage.zeroize();
            return s;
        }

        static void return_stack(ContextualStack *s) {
            delete s;
        }
    };

    inline ContextualStack *get_stack(StackType type, void (*entry)(intptr_t)) {
        switch (type) {
            case StackType::STACK_TYPE_PTHREAD:
                return nullptr;
            case StackType::STACK_TYPE_SMALL:
                return StackFactory<SmallStackClass>::get_stack(entry);
            case StackType::STACK_TYPE_NORMAL:
                return StackFactory<NormalStackClass>::get_stack(entry);
            case StackType::STACK_TYPE_LARGE:
                return StackFactory<LargeStackClass>::get_stack(entry);
            case StackType::STACK_TYPE_MAIN:
                return StackFactory<MainStackClass>::get_stack(entry);
        }
        return nullptr;
    }

    inline void return_stack(ContextualStack *s) {
        if (nullptr == s) {
            return;
        }
        switch (s->stacktype) {
            case StackType::STACK_TYPE_PTHREAD:
                assert(false);
                return;
            case StackType::STACK_TYPE_SMALL:
                return StackFactory<SmallStackClass>::return_stack(s);
            case StackType::STACK_TYPE_NORMAL:
                return StackFactory<NormalStackClass>::return_stack(s);
            case StackType::STACK_TYPE_LARGE:
                return StackFactory<LargeStackClass>::return_stack(s);
            case StackType::STACK_TYPE_MAIN:
                return StackFactory<MainStackClass>::return_stack(s);
        }
    }

    inline void jump_stack(ContextualStack *from, ContextualStack *to) {
        ::fiber_jump_fcontext(&from->context, to->context, 0/*not skip remained*/);
    }

}  // namespace turbo::fiber_internal

namespace turbo {
    using LargeStackClassType = turbo::fiber_internal::StackFactory<turbo::fiber_internal::LargeStackClass>::Wrapper;

    template<>
    struct ObjectPoolTraits<LargeStackClassType> : public ObjectPoolTraitsBase<LargeStackClassType> {

        static constexpr size_t block_max_items() {
            return 64;
        }

        static constexpr size_t free_chunk_max_items() {
            return (FiberStackConfig::tc_stack_small <= 0 ? 0 : FiberStackConfig::tc_stack_small);
        }

        static bool validate(const LargeStackClassType *ptr) {
            return  ptr->context != nullptr;
        }
    };

    using NormalStackClassType = turbo::fiber_internal::StackFactory<turbo::fiber_internal::NormalStackClass>::Wrapper;
    template<>
    struct ObjectPoolTraits<NormalStackClassType> : public ObjectPoolTraitsBase<NormalStackClassType> {

        static constexpr size_t block_max_items() {
            return 64;
        }

        static constexpr size_t free_chunk_max_items() {
            return (FiberStackConfig::tc_stack_small <= 0 ? 0 : FiberStackConfig::tc_stack_small);
        }

        static bool validate(const NormalStackClassType *ptr) {
            return  ptr->context != nullptr;
        }
    };

    using SmallStackClassType = turbo::fiber_internal::StackFactory<turbo::fiber_internal::SmallStackClass>::Wrapper;
    template<>
    struct ObjectPoolTraits<SmallStackClassType> : public ObjectPoolTraitsBase<SmallStackClassType> {

        static constexpr size_t block_max_items() {
            return 64;
        }

        static constexpr size_t free_chunk_max_items() {
            return (FiberStackConfig::tc_stack_small <= 0 ? 0 : FiberStackConfig::tc_stack_small);
        }

        static bool validate(const SmallStackClassType *ptr) {
            return  ptr->context != nullptr;
        }
    };

}
#endif  // TURBO_FIBER_INTERNAL_STACK_H_
