
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************///
// -----------------------------------------------------------------------------
// File: thread_annotations.h
// -----------------------------------------------------------------------------
//
// This header file contains macro definitions for thread safety annotations
// that allow developers to document the locking policies of multi-threaded
// code. The annotations can also help program analysis tools to identify
// potential thread safety issues.
//
// These annotations are implemented using compiler attributes. Using the macros
// defined here instead of raw attributes allow for portability and future
// compatibility.
//
// When referring to mutexes in the arguments of the attributes, you should
// use variable names or more complex expressions (e.g. my_object->mutex_)
// that evaluate to a concrete mutex object whenever possible. If the mutex
// you want to refer to is not in scope, you may use a member pointer
// (e.g. &MyClass::mutex_) to refer to a mutex in some (unknown) object.

#ifndef TURBO_THREAD_THREAD_ANNOTATIONS_H_
#define TURBO_THREAD_THREAD_ANNOTATIONS_H_

#include "turbo/base/profile.h"

#if defined(__clang__)
#define TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(x) __attribute__((x))
#else
#define TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(x)  // no-op
#endif
// TURBO_GUARDED_BY()
//
// Documents if a shared field or global variable needs to be protected by a
// mutex. TURBO_GUARDED_BY() allows the user to specify a particular mutex that
// should be held when accessing the annotated variable.
//
// Although this annotation (and TURBO_PT_GUARDED_BY, below) cannot be applied to
// local variables, a local variable and its associated mutex can often be
// combined into a small class or struct, thereby allowing the annotation.
//
// Example:
//
//   class Foo {
//     mutex mu_;
//     int p1_ TURBO_GUARDED_BY(mu_);
//     ...
//   };
#define TURBO_GUARDED_BY(x) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(guarded_by(x))

// TURBO_PT_GUARDED_BY()
//
// Documents if the memory location pointed to by a pointer should be guarded
// by a mutex when dereferencing the pointer.
//
// Example:
//   class Foo {
//     mutex mu_;
//     int *p1_ TURBO_PT_GUARDED_BY(mu_);
//     ...
//   };
//
// Note that a pointer variable to a shared memory location could itself be a
// shared variable.
//
// Example:
//
//   // `q_`, guarded by `mu1_`, points to a shared memory location that is
//   // guarded by `mu2_`:
//   int *q_ TURBO_GUARDED_BY(mu1_) TURBO_PT_GUARDED_BY(mu2_);
#define TURBO_PT_GUARDED_BY(x) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(pt_guarded_by(x))

// TURBO_ACQUIRED_AFTER() / TURBO_ACQUIRED_BEFORE()
//
// Documents the acquisition order between locks that can be held
// simultaneously by a thread. For any two locks that need to be annotated
// to establish an acquisition order, only one of them needs the annotation.
// (i.e. You don't have to annotate both locks with both TURBO_ACQUIRED_AFTER
// and TURBO_ACQUIRED_BEFORE.)
//
// As with TURBO_GUARDED_BY, this is only applicable to mutexes that are shared
// fields or global variables.
//
// Example:
//
//   mutex m1_;
//   mutex m2_ TURBO_ACQUIRED_AFTER(m1_);
#define TURBO_ACQUIRED_AFTER(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(acquired_after(__VA_ARGS__))

#define TURBO_ACQUIRED_BEFORE(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(acquired_before(__VA_ARGS__))

// TURBO_EXCLUSIVE_LOCKS_REQUIRED() / TURBO_SHARED_LOCKS_REQUIRED()
//
// Documents a function that expects a mutex to be held prior to entry.
// The mutex is expected to be held both on entry to, and exit from, the
// function.
//
// An exclusive lock allows read-write access to the guarded data member(s), and
// only one thread can acquire a lock exclusively at any one time. A shared lock
// allows read-only access, and any number of threads can acquire a shared lock
// concurrently.
//
// Generally, non-const methods should be annotated with
// TURBO_EXCLUSIVE_LOCKS_REQUIRED, while const methods should be annotated with
// TURBO_SHARED_LOCKS_REQUIRED.
//
// Example:
//
//   mutex mu1, mu2;
//   int a TURBO_GUARDED_BY(mu1);
//   int b TURBO_GUARDED_BY(mu2);
//
//   void foo() TURBO_EXCLUSIVE_LOCKS_REQUIRED(mu1, mu2) { ... }
//   void bar() const TURBO_SHARED_LOCKS_REQUIRED(mu1, mu2) { ... }
#define TURBO_EXCLUSIVE_LOCKS_REQUIRED(...)   \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE( \
      exclusive_locks_required(__VA_ARGS__))

#define TURBO_SHARED_LOCKS_REQUIRED(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(shared_locks_required(__VA_ARGS__))

// TURBO_LOCKS_EXCLUDED()
//
// Documents the locks acquired in the body of the function. These locks
// cannot be held when calling this function.
#define TURBO_LOCKS_EXCLUDED(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(locks_excluded(__VA_ARGS__))

// TURBO_LOCK_RETURNED()
//
// Documents a function that returns a mutex without acquiring it.  For example,
// a public getter method that returns a pointer to a private mutex should
// be annotated with TURBO_LOCK_RETURNED.
#define TURBO_LOCK_RETURNED(x) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(lock_returned(x))

// TURBO_LOCKABLE
//
// Documents if a class/type is a lockable type (such as the `mutex` class).
#define TURBO_LOCKABLE TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(lockable)

// TURBO_SCOPED_LOCKABLE
//
// Documents if a class does RAII locking (such as the `mutex_lock` class).
// The constructor should use `LOCK_FUNCTION()` to specify the mutex that is
// acquired, and the destructor should use `UNLOCK_FUNCTION()` with no
// arguments; the analysis will assume that the destructor unlocks whatever the
// constructor locked.
#define TURBO_SCOPED_LOCKABLE \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(scoped_lockable)

// TURBO_EXCLUSIVE_LOCK_FUNCTION()
//
// Documents functions that acquire a lock in the body of a function, and do
// not release it.
#define TURBO_EXCLUSIVE_LOCK_FUNCTION(...)    \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE( \
      exclusive_lock_function(__VA_ARGS__))

// TURBO_SHARED_LOCK_FUNCTION()
//
// Documents functions that acquire a shared (reader) lock in the body of a
// function, and do not release it.
#define TURBO_SHARED_LOCK_FUNCTION(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(shared_lock_function(__VA_ARGS__))

// TURBO_UNLOCK_FUNCTION()
//
// Documents functions that expect a lock to be held on entry to the function,
// and release it in the body of the function.
#define TURBO_UNLOCK_FUNCTION(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(unlock_function(__VA_ARGS__))

// TURBO_EXCLUSIVE_TRYLOCK_FUNCTION() / TURBO_SHARED_TRYLOCK_FUNCTION()
//
// Documents functions that try to acquire a lock, and return success or failure
// (or a non-boolean value that can be interpreted as a boolean).
// The first argument should be `true` for functions that return `true` on
// success, or `false` for functions that return `false` on success. The second
// argument specifies the mutex that is locked on success. If unspecified, this
// mutex is assumed to be `this`.
#define TURBO_EXCLUSIVE_TRYLOCK_FUNCTION(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE( \
      exclusive_trylock_function(__VA_ARGS__))

#define TURBO_SHARED_TRYLOCK_FUNCTION(...)    \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE( \
      shared_trylock_function(__VA_ARGS__))

// TURBO_ASSERT_EXCLUSIVE_LOCK() / TURBO_ASSERT_SHARED_LOCK()
//
// Documents functions that dynamically check to see if a lock is held, and fail
// if it is not held.
#define TURBO_ASSERT_EXCLUSIVE_LOCK(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(assert_exclusive_lock(__VA_ARGS__))

#define TURBO_ASSERT_SHARED_LOCK(...) \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(assert_shared_lock(__VA_ARGS__))

// TURBO_NO_THREAD_SAFETY_ANALYSIS
//
// Turns off thread safety checking within the body of a particular function.
// This annotation is used to mark functions that are known to be correct, but
// the locking behavior is more complicated than the analyzer can handle.
#define TURBO_NO_THREAD_SAFETY_ANALYSIS \
  TURBO_INTERNAL_THREAD_ANNOTATION_ATTRIBUTE(no_thread_safety_analysis)

//------------------------------------------------------------------------------
// Tool-Supplied Annotations
//------------------------------------------------------------------------------

// TURBO_TS_UNCHECKED should be placed around lock expressions that are not valid
// C++ syntax, but which are present for documentation purposes.  These
// annotations will be ignored by the analysis.
#define TURBO_TS_UNCHECKED(x) ""

// TURBO_TS_FIXME is used to mark lock expressions that are not valid C++ syntax.
// It is used by automated tools to mark and disable invalid expressions.
// The annotation should either be fixed, or changed to TURBO_TS_UNCHECKED.
#define TURBO_TS_FIXME(x) ""

// Like TURBO_NO_THREAD_SAFETY_ANALYSIS, this turns off checking within the body
// of a particular function.  However, this attribute is used to mark functions
// that are incorrect and need to be fixed.  It is used by automated tools to
// avoid breaking the build when the analysis is updated.
// Code owners are expected to eventually fix the routine.
#define TURBO_NO_THREAD_SAFETY_ANALYSIS_FIXME TURBO_NO_THREAD_SAFETY_ANALYSIS

// Similar to TURBO_NO_THREAD_SAFETY_ANALYSIS_FIXME, this macro marks a
// TURBO_GUARDED_BY annotation that needs to be fixed, because it is producing
// thread safety warning. It disables the TURBO_GUARDED_BY.
#define TURBO_GUARDED_BY_FIXME(x)

// Disables warnings for a single read operation.  This can be used to avoid
// warnings when it is known that the read is not actually involved in a race,
// but the compiler cannot confirm that.
#define TURBO_TS_UNCHECKED_READ(x) turbo::base_internal::ts_unchecked_read(x)

namespace turbo::thread_internal {

    // Takes a reference to a guarded data member, and returns an unguarded
    // reference.
    // Do not used this function directly, use TURBO_TS_UNCHECKED_READ instead.
    template<typename T>
    TURBO_FORCE_INLINE const T &ts_unchecked_read(const T &v) TURBO_NO_THREAD_SAFETY_ANALYSIS {
        return v;
    }

    template<typename T>
    TURBO_FORCE_INLINE T &ts_unchecked_read(T &v) TURBO_NO_THREAD_SAFETY_ANALYSIS {
        return v;
    }


}  // namespace turbo::thread_internal

#endif  // TURBO_THREAD_THREAD_ANNOTATIONS_H_
