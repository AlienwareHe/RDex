/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_RUNTIME_BASE_MACROS_H_
#define ART_RUNTIME_BASE_MACROS_H_

#include <stddef.h>  // for size_t
#include <unistd.h>  // for TEMP_FAILURE_RETRY

// bionic and glibc both have TEMP_FAILURE_RETRY, but eg Mac OS' libc doesn't.
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({ \
  decltype(exp) _rc; \
  do { \
    _rc = (exp); \
  } while (_rc == -1 && errno == EINTR); \
  _rc; })
#endif

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

// C++11 final and override keywords that were introduced in GCC version 4.7.
#if defined(__clang__) || GCC_VERSION >= 40700
#define OVERRIDE override
#define FINAL final
#else
#define OVERRIDE
#define FINAL
#endif

// Declare a friend relationship in a class with a test. Used rather that FRIEND_TEST to avoid
// globally importing gtest/gtest.h into the main ART header files.
#define ART_FRIEND_TEST(test_set_name, individual_test)\
friend class test_set_name##_##individual_test##_Test

// Declare a friend relationship in a class with a typed test.
#define ART_FRIEND_TYPED_TEST(test_set_name, individual_test)\
template<typename T> ART_FRIEND_TEST(test_set_name, individual_test)

// DISALLOW_COPY_AND_ASSIGN disallows the copy and operator= functions. It goes in the private:
// declarations in a class.
#if !defined(DISALLOW_COPY_AND_ASSIGN)
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;  \
  void operator=(const TypeName&) = delete
#endif

// A macro to disallow all the implicit constructors, namely the default constructor, copy
// constructor and operator= functions.
//
// This should be used in the private: declarations for a class that wants to prevent anyone from
// instantiating it. This is especially useful for classes containing only static methods.
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete;  \
  DISALLOW_COPY_AND_ASSIGN(TypeName)

// A macro to disallow new and delete operators for a class. It goes in the private: declarations.
// NOTE: Providing placement new (and matching delete) for constructing container elements.
#define DISALLOW_ALLOCATION() \
  public: \
    NO_RETURN ALWAYS_INLINE void operator delete(void*, size_t) { UNREACHABLE(); } \
    ALWAYS_INLINE void* operator new(size_t, void* ptr) noexcept { return ptr; } \
    ALWAYS_INLINE void operator delete(void*, void*) noexcept { } \
  private: \
    void* operator new(size_t) = delete

// The arraysize(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.  If you use arraysize on
// a pointer by mistake, you will get a compile-time error.
//
// One caveat is that arraysize() doesn't accept any array of an
// anonymous type or a type defined inside a function.  In these rare
// cases, you have to use the unsafe ARRAYSIZE_UNSAFE() macro below.  This is
// due to a limitation in C++'s template system.  The limitation might
// eventually be removed, but it hasn't happened yet.

// This template function declaration is used in defining arraysize.
// Note that the function doesn't need an implementation, as we only
// use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

#define arraysize(array) (sizeof(ArraySizeHelper(array)))

// ARRAYSIZE_UNSAFE performs essentially the same calculation as arraysize,
// but can be used on anonymous types or types defined inside
// functions.  It's less safe than arraysize as it accepts some
// (although not all) pointers.  Therefore, you should use arraysize
// whenever possible.
//
// The expression ARRAYSIZE_UNSAFE(a) is a compile-time constant of type
// size_t.
//
// ARRAYSIZE_UNSAFE catches a few type errors.  If you see a compiler error
//
//   "warning: division by zero in ..."
//
// when using ARRAYSIZE_UNSAFE, you are (wrongfully) giving it a pointer.
// You should only use ARRAYSIZE_UNSAFE on statically allocated arrays.
//
// The following comments are on the implementation details, and can
// be ignored by the users.
//
// ARRAYSIZE_UNSAFE(arr) works by inspecting sizeof(arr) (the # of bytes in
// the array) and sizeof(*(arr)) (the # of bytes in one array
// element).  If the former is divisible by the latter, perhaps arr is
// indeed an array, in which case the division result is the # of
// elements in the array.  Otherwise, arr cannot possibly be an array,
// and we generate a compiler error to prevent the code from
// compiling.
//
// Since the size of bool is implementation-defined, we need to cast
// !(sizeof(a) & sizeof(*(a))) to size_t in order to ensure the final
// result has type size_t.
//
// This macro is not perfect as it wrongfully accepts certain
// pointers, namely where the pointer size is divisible by the pointee
// size.  Since all our code has to go through a 32-bit compiler,
// where a pointer is 4 bytes, this means all pointers to a type whose
// size is 3 or greater than 4 will be (righteously) rejected.
#define ARRAYSIZE_UNSAFE(a) \
  ((sizeof(a) / sizeof(*(a))) / static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

#define SIZEOF_MEMBER(t, f) sizeof((reinterpret_cast<t*>(4096))->f)

#define OFFSETOF_MEMBER(t, f) \
  (reinterpret_cast<uintptr_t>(&reinterpret_cast<t*>(16)->f) - static_cast<uintptr_t>(16u)) // NOLINT

#define OFFSETOF_MEMBERPTR(t, f) \
  (reinterpret_cast<uintptr_t>(&(reinterpret_cast<t*>(16)->*f)) - static_cast<uintptr_t>(16)) // NOLINT

#define PACKED(x) __attribute__ ((__aligned__(x), __packed__))

#define LIKELY(x)       __builtin_expect((x), true)
#define UNLIKELY(x)     __builtin_expect((x), false)

// Stringify the argument.
#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

#ifndef NDEBUG
#define ALWAYS_INLINE
#else
#define ALWAYS_INLINE  __attribute__ ((always_inline))
#endif

#ifdef __clang__
/* clang doesn't like attributes on lambda functions */
#define ALWAYS_INLINE_LAMBDA
#else
#define ALWAYS_INLINE_LAMBDA ALWAYS_INLINE
#endif

#define NO_INLINE __attribute__ ((noinline))

#if defined (__APPLE__)
#define HOT_ATTR
#define COLD_ATTR
#else
#define HOT_ATTR __attribute__ ((hot))
#define COLD_ATTR __attribute__ ((cold))
#endif

#define PURE __attribute__ ((__pure__))
#define WARN_UNUSED __attribute__((warn_unused_result))

// A deprecated function to call to create a false use of the parameter, for example:
//   int foo(int x) { UNUSED(x); return 10; }
// to avoid compiler warnings. Going forward we prefer ATTRIBUTE_UNUSED.
template<typename... T> void UNUSED(const T&...) {}

// An attribute to place on a parameter to a function, for example:
//   int foo(int x ATTRIBUTE_UNUSED) { return 10; }
// to avoid compiler warnings.
#define ATTRIBUTE_UNUSED __attribute__((__unused__))

// Define that a position within code is unreachable, for example:
//   int foo () { LOG(FATAL) << "Don't call me"; UNREACHABLE(); }
// without the UNREACHABLE a return statement would be necessary.
#define UNREACHABLE  __builtin_unreachable

// Add the C++11 noreturn attribute.
#define NO_RETURN [[ noreturn ]]  // NOLINT[whitespace/braces] [5]

// The FALLTHROUGH_INTENDED macro can be used to annotate implicit fall-through
// between switch labels:
//  switch (x) {
//    case 40:
//    case 41:
//      if (truth_is_out_there) {
//        ++x;
//        FALLTHROUGH_INTENDED;  // Use instead of/along with annotations in
//                               // comments.
//      } else {
//        return x;
//      }
//    case 42:
//      ...
//
//  As shown in the example above, the FALLTHROUGH_INTENDED macro should be
//  followed by a semicolon. It is designed to mimic control-flow statements
//  like 'break;', so it can be placed in most places where 'break;' can, but
//  only if there are no statements on the execution path between it and the
//  next switch label.
//
//  When compiled with clang in C++11 mode, the FALLTHROUGH_INTENDED macro is
//  expanded to [[clang::fallthrough]] attribute, which is analysed when
//  performing switch labels fall-through diagnostic ('-Wimplicit-fallthrough').
//  See clang documentation on language extensions for details:
//  http://clang.llvm.org/docs/LanguageExtensions.html#clang__fallthrough
//
//  When used with unsupported compilers, the FALLTHROUGH_INTENDED macro has no
//  effect on diagnostics.
//
//  In either case this macro has no effect on runtime behavior and performance
//  of code.
#if defined(__clang__) && __cplusplus >= 201103L && defined(__has_warning)
#if __has_feature(cxx_attributes) && __has_warning("-Wimplicit-fallthrough")
#define FALLTHROUGH_INTENDED [[clang::fallthrough]]  // NOLINT
#endif
#endif

#ifndef FALLTHROUGH_INTENDED
#define FALLTHROUGH_INTENDED do { } while (0)
#endif

// Annotalysis thread-safety analysis support.
#if defined(__SUPPORT_TS_ANNOTATION__) || defined(__clang__)
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   // no-op
#endif

#define ACQUIRED_AFTER(...) THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))
#define ACQUIRED_BEFORE(...) THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))
#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))
#define GUARDED_VAR THREAD_ANNOTATION_ATTRIBUTE__(guarded)
#define LOCK_RETURNED(x) THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))
#define NO_THREAD_SAFETY_ANALYSIS THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)
#define PT_GUARDED_BY(x)
// THREAD_ANNOTATION_ATTRIBUTE__(point_to_guarded_by(x))
#define PT_GUARDED_VAR THREAD_ANNOTATION_ATTRIBUTE__(point_to_guarded)
#define SCOPED_LOCKABLE THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#if defined(__clang__)
#define EXCLUSIVE_LOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(exclusive_lock_function(__VA_ARGS__))
#define EXCLUSIVE_TRYLOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(exclusive_trylock_function(__VA_ARGS__))
#define SHARED_LOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(shared_lock_function(__VA_ARGS__))
#define SHARED_TRYLOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(shared_trylock_function(__VA_ARGS__))
#define UNLOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(unlock_function(__VA_ARGS__))
#define REQUIRES(...) THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))
#define SHARED_REQUIRES(...) THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))
#define CAPABILITY(...) THREAD_ANNOTATION_ATTRIBUTE__(capability(__VA_ARGS__))
#define SHARED_CAPABILITY(...) THREAD_ANNOTATION_ATTRIBUTE__(shared_capability(__VA_ARGS__))
#define ASSERT_CAPABILITY(...) THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(__VA_ARGS__))
#define ASSERT_SHARED_CAPABILITY(...) THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(__VA_ARGS__))
#define RETURN_CAPABILITY(...) THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(__VA_ARGS__))
#define TRY_ACQUIRE(...) THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))
#define TRY_ACQUIRE_SHARED(...) THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))
#define ACQUIRE(...) THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))
#define ACQUIRE_SHARED(...) THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))
#define RELEASE(...) THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))
#define RELEASE_SHARED(...) THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))
#define SCOPED_CAPABILITY THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)
#else
#define EXCLUSIVE_LOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(exclusive_lock(__VA_ARGS__))
#define EXCLUSIVE_TRYLOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(exclusive_trylock(__VA_ARGS__))
#define SHARED_LOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(shared_lock(__VA_ARGS__))
#define SHARED_TRYLOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(shared_trylock(__VA_ARGS__))
#define UNLOCK_FUNCTION(...) THREAD_ANNOTATION_ATTRIBUTE__(unlock(__VA_ARGS__))
#define REQUIRES(...)
#define SHARED_REQUIRES(...)
#define CAPABILITY(...)
#define SHARED_CAPABILITY(...)
#define ASSERT_CAPABILITY(...)
#define ASSERT_SHARED_CAPABILITY(...)
#define RETURN_CAPABILITY(...)
#define TRY_ACQUIRE(...)
#define TRY_ACQUIRE_SHARED(...)
#define ACQUIRE(...)
#define ACQUIRE_SHARED(...)
#define RELEASE(...)
#define RELEASE_SHARED(...)
#define SCOPED_CAPABILITY
#endif

#define LOCKABLE CAPABILITY("mutex")
#define SHARED_LOCKABLE SHARED_CAPABILITY("mutex")

#endif  // ART_RUNTIME_BASE_MACROS_H_