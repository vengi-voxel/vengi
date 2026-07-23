/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/String.h"
#include "core/collection/StringSet.h"
#include "core/concurrent/ReadWriteLock.h"

namespace core {

/**
 * @brief Thread-safe intern pool for immutable, high-reuse strings.
 *
 * Use only for a bounded vocabulary that is hit often (command names, shortcut
 * labels, stable UI titles, shader attribute names, ...). Do not intern paths,
 * per-row ImGui ids, or other high-cardinality / one-shot strings.
 *
 * Returned @c const String& / @c c_str() pointers are for short-lived locals in
 * the current call (e.g. an ImGui label this frame). Do not store them in
 * members or other long-lived state -- own a @c core::String instead.
 *
 * The underlying set has a fixed capacity; inserting past that capacity asserts.
 * There is no @c clear(): the pool is meant to grow to a small plateau and stay.
 *
 * @note @c StringCacheHash is the map-key type with a frozen hash; this class is
 * the string pool built on top of it.
 */
class StringCache {
private:
	mutable ReadWriteLock _lock;
	StringSet _strings core_thread_guarded_by(_lock);

public:
	explicit StringCache(int maxSize = 4096);

	/**
	 * @return Stable reference to the interned string. Empty input returns
	 * @c String::Empty without inserting.
	 */
	const String &get(const char *str);
	const String &get(const String &str);

	/**
	 * @brief Format into a fixed buffer, then intern the result.
	 */
	const String &getFormat(CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(2);

	const char *c_str(const char *str);
	const char *c_str(const String &str);

	size_t size() const;
	size_t capacity() const;
};

} // namespace core
