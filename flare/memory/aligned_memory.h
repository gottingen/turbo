// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// aligned_memory is a POD type that gives you a portable way to specify static
// or local stack data of a given alignment and size. For example, if you need
// static storage for a class, but you want manual control over when the object
// is constructed and destructed (you don't want static initialization and
// destruction), use aligned_memory:
//
//   static aligned_memory<sizeof(MyClass), FLARE_ALIGN_OF(MyClass)> my_class;
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

#ifndef FLARE_MEMORY_ALIGNED_MEMORY_H_
#define FLARE_MEMORY_ALIGNED_MEMORY_H_

#include "flare/base/profile.h"
#include <cstdint>

#if defined(FLARE_COMPILER_MSVC)
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace flare {

    // aligned_memory is specialized for all supported alignments.
    // Make sure we get a compiler error if someone uses an unsupported alignment.
    template<size_t Size, size_t ByteAlignment>
    struct aligned_memory {
    };

#define FLARE_DECL_ALIGNED_MEMORY(byte_alignment) \
    template <size_t Size> \
    class aligned_memory<Size, byte_alignment> { \
     public: \
      FLARE_ALIGN_AS(byte_alignment) uint8_t data_[Size]; \
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
    // does not understand FLARE_ALIGN_AS(FLARE_ALIGN_OF(Type)) or FLARE_ALIGN_AS(template_param).
    // Greater than 4096 alignment is not supported by some compilers, so 4096 is
    // the maximum specified here.
    FLARE_DECL_ALIGNED_MEMORY(1);

    FLARE_DECL_ALIGNED_MEMORY(2);

    FLARE_DECL_ALIGNED_MEMORY(4);

    FLARE_DECL_ALIGNED_MEMORY(8);

    FLARE_DECL_ALIGNED_MEMORY(16);

    FLARE_DECL_ALIGNED_MEMORY(32);

    FLARE_DECL_ALIGNED_MEMORY(64);

    FLARE_DECL_ALIGNED_MEMORY(128);

    FLARE_DECL_ALIGNED_MEMORY(256);

    FLARE_DECL_ALIGNED_MEMORY(512);

    FLARE_DECL_ALIGNED_MEMORY(1024);

    FLARE_DECL_ALIGNED_MEMORY(2048);

    FLARE_DECL_ALIGNED_MEMORY(4096);

#undef FLARE_DECL_ALIGNED_MEMORY

    FLARE_EXPORT void *aligned_alloc(size_t size, size_t alignment);

    inline void aligned_free(void *ptr) {
#if defined(FLARE_COMPILER_MSVC)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }

    // Deleter for use with scoped_ptr. E.g., use as
    //   scoped_ptr<Foo, flare::aligned_free_deleter> foo;
    struct aligned_free_deleter {
        inline void operator()(void *ptr) const {
            aligned_free(ptr);
        }
    };

}  // namespace flare

#endif  // FLARE_MEMORY_ALIGNED_MEMORY_H_
