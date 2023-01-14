// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// aligned_memory is a POD type that gives you a portable way to specify static
// or local stack data of a given alignment and size. For example, if you need
// static storage for a class, but you want manual control over when the object
// is constructed and destructed (you don't want static initialization and
// destruction), use aligned_memory:
//
//   static aligned_memory<sizeof(MyClass), TURBO_ALIGN_OF(MyClass)> my_class;
//
//   // ... at runtime:
//   new(my_class.void_data()) MyClass();
//
//   // ... use it:
//   MyClass* mc = my_class.data_as<MyClass>();
//
//   // ... later, to destruct my_class:
//   my_class.data_as<MyClass>()->MyClass::~MyClass();
//
// Alternatively, a runtime sized aligned allocation can be created:
//
//   float* my_array = static_cast<float*>(AlignedAlloc(size, alignment));
//
//   // ... later, to release the memory:
//   AlignedFree(my_array);
//
// Or using scoped_ptr:
//
//   scoped_ptr<float, aligned_free_deleter> my_array(
//       static_cast<float*>(AlignedAlloc(size, alignment)));

#ifndef TURBO_MEMORY_ALIGNED_MEMORY_H_
#define TURBO_MEMORY_ALIGNED_MEMORY_H_

#include "turbo/base/profile.h"
#include <cstdint>

#if defined(TURBO_COMPILER_MSVC)
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace turbo {

    // aligned_memory is specialized for all supported alignments.
    // Make sure we get a compiler error if someone uses an unsupported alignment.
    template<size_t Size, size_t ByteAlignment>
    struct aligned_memory {
    };

#define TURBO_DECL_ALIGNED_MEMORY(byte_alignment) \
    template <size_t Size> \
    class aligned_memory<Size, byte_alignment> { \
     public: \
      TURBO_ALIGN_AS(byte_alignment) uint8_t data_[Size]; \
      void* void_data() { return static_cast<void*>(data_); } \
      const void* void_data() const { \
        return static_cast<const void*>(data_); \
      } \
      template<typename Type> \
      Type* data_as() { return static_cast<Type*>(void_data()); } \
      template<typename Type> \
      const Type* data_as() const { \
        return static_cast<const Type*>(void_data()); \
      } \
     private: \
      void* operator new(size_t); \
      void operator delete(void*); \
    }

    // Specialization for all alignments is required because MSVC (as of VS 2008)
    // does not understand TURBO_ALIGN_AS(TURBO_ALIGN_OF(Type)) or TURBO_ALIGN_AS(template_param).
    // Greater than 4096 alignment is not supported by some compilers, so 4096 is
    // the maximum specified here.
    TURBO_DECL_ALIGNED_MEMORY(1);

    TURBO_DECL_ALIGNED_MEMORY(2);

    TURBO_DECL_ALIGNED_MEMORY(4);

    TURBO_DECL_ALIGNED_MEMORY(8);

    TURBO_DECL_ALIGNED_MEMORY(16);

    TURBO_DECL_ALIGNED_MEMORY(32);

    TURBO_DECL_ALIGNED_MEMORY(64);

    TURBO_DECL_ALIGNED_MEMORY(128);

    TURBO_DECL_ALIGNED_MEMORY(256);

    TURBO_DECL_ALIGNED_MEMORY(512);

    TURBO_DECL_ALIGNED_MEMORY(1024);

    TURBO_DECL_ALIGNED_MEMORY(2048);

    TURBO_DECL_ALIGNED_MEMORY(4096);

#undef TURBO_DECL_ALIGNED_MEMORY

    TURBO_EXPORT void *aligned_alloc(size_t size, size_t alignment);

    inline void aligned_free(void *ptr) {
#if defined(TURBO_COMPILER_MSVC)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    // Deleter for use with scoped_ptr. E.g., use as
    //   scoped_ptr<Foo, turbo::aligned_free_deleter> foo;
    struct aligned_free_deleter {
        inline void operator()(void *ptr) const {
            aligned_free(ptr);
        }
    };

}  // namespace turbo

#endif  // TURBO_MEMORY_ALIGNED_MEMORY_H_
