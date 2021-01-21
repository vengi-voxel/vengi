/**
 * @file
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
#ifdef __clang__
#define THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x) // no-op
#endif

/**
 * @brief is an attribute on classes, which specifies that objects of the class can be used as a capability. The string
 * argument specifies the kind of capability in error messages, e.g. "mutex".
 */
#define core_thread_capability(x) THREAD_ANNOTATION_ATTRIBUTE__(capability(x))
/**
 * @brief is an attribute on classes that implement RAII-style locking, in which a capability is acquired in the
 * constructor, and released in the destructor. Such classes require special handling because the constructor and
 * destructor refer to the capability via different names
 */
#define core_thread_scoped_capability THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)
/**
 * @brief is an attribute on data members, which declares that the data member is protected by the given capability.
 * Read operations on the data require shared access, while write operations require exclusive access.
 */
#define core_thread_guarded_by(x) THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))
/**
 * @brief is similar to @c core_guarded_by, but is intended for use on pointers and smart pointers. There is no
 * constraint on the data member itself, but the data that it points to is protected by the given capability.
 */
#define core_thread_pt_guarded_by(x) THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))
/**
 * @brief attribute on member declarations, specifically declarations of mutexes or other capabilities. These
 * declarations enforce a particular order in which the mutexes must be acquired, in order to prevent deadlock.
 */
#define core_thread_acquired_before(...) THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))
/**
 * @brief attribute on member declarations, specifically declarations of mutexes or other capabilities. These
 * declarations enforce a particular order in which the mutexes must be acquired, in order to prevent deadlock.
 */
#define core_thread_acquired_after(...) THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))
/**
 * @brief is an attribute on functions or methods, which declares that the calling thread must have exclusive access to
 * the given capabilities. More than one capability may be specified. The capabilities must be held on entry to the
 * function, and must still be held on exit.
 */
#define core_thread_requires(...) THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))
#define core_thread_requires_shared(...) THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))
/**
 * @brief attributes on functions or methods declaring that the function acquires a capability, but does not release it.
 * The given capability must not be held on entry, and will be held on exit
 */
#define core_thread_acquire(...) THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))
#define core_thread_acquire_shared(...) THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))
/**
 * @brief declare that the function releases the given capability. The capability must be held on entry, and will no
 * longer be held on exit.
 */
#define core_thread_release(...) THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))
#define core_thread_release_shared(...) THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))
#define core_thread_release_generic(...) THREAD_ANNOTATION_ATTRIBUTE__(release_generic_capability(__VA_ARGS__))
#define core_thread_try_acquire(...) THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))
#define core_thread_try_acquire_shared(...) THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))
/**
 * @brief is an attribute on functions or methods, which declares that the caller must not hold the given capabilities.
 * This annotation is used to prevent deadlock. Many mutex implementations are not re-entrant, so deadlock can occur if
 * the function acquires the mutex a second time.
 */
#define core_thread_excludes(...) THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))
#define core_thread_assert_capability(x) THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))
#define core_thread_assert_shared_capability(x) THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))
/**
 * @brief is an attribute on functions or methods, which declares that the function returns a reference to the given
 * capability. It is used to annotate getter methods that return mutexes.
 */
#define core_thread_return_capability(x) THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))
#define core_thread_no_thread_safety_analysis THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

namespace core {

extern uint32_t cpus();
extern uint32_t halfcpus();

extern bool setThreadName(const char *name);

enum class ThreadPriority { High, Normal, Low };

extern size_t getThreadId();
extern void setThreadPriority(ThreadPriority prio);

}
