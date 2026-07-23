/**
 * @file
 */

#include "core/StringCache.h"
#include "core/Assert.h"
#include "core/StringCacheHash.h"
#include "core/concurrent/ReadWriteLock.h"
#include <SDL3/SDL_stdinc.h>
#include <stdarg.h>

namespace core {

namespace {

const String &internMiss(StringSet &strings, const StringCacheHash &key) {
	strings.insert(key);
	auto i = strings.find(key);
	core_assert_msg(i != strings.end(), "StringCache insert failed (pool full? size %i / %i)", (int)strings.size(),
					(int)strings.capacity());
	return i->key;
}

} // namespace

StringCache::StringCache(int maxSize) : _strings(maxSize) {
}

const String &StringCache::get(const char *str) {
	if (str == nullptr || str[0] == '\0') {
		return String::Empty;
	}
	const StringCacheHash key(str);
	{
		ScopedReadLock read(_lock);
		auto i = _strings.find(key);
		if (i != _strings.end()) {
			return i->key;
		}
	}
	ScopedWriteLock write(_lock);
	auto i = _strings.find(key);
	if (i != _strings.end()) {
		return i->key;
	}
	return internMiss(_strings, key);
}

const String &StringCache::get(const String &str) {
	if (str.empty()) {
		return String::Empty;
	}
	const StringCacheHash key(str);
	{
		ScopedReadLock read(_lock);
		auto i = _strings.find(key);
		if (i != _strings.end()) {
			return i->key;
		}
	}
	ScopedWriteLock write(_lock);
	auto i = _strings.find(key);
	if (i != _strings.end()) {
		return i->key;
	}
	return internMiss(_strings, key);
}

const String &StringCache::getFormat(const char *fmt, ...) {
	if (fmt == nullptr || fmt[0] == '\0') {
		return String::Empty;
	}
	char buf[512];
	va_list args;
	va_start(args, fmt);
	const int written = SDL_vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	if (written <= 0) {
		return String::Empty;
	}
	buf[sizeof(buf) - 1] = '\0';
	return get(buf);
}

const char *StringCache::c_str(const char *str) {
	return get(str).c_str();
}

const char *StringCache::c_str(const String &str) {
	return get(str).c_str();
}

size_t StringCache::size() const {
	ScopedReadLock read(_lock);
	return _strings.size();
}

size_t StringCache::capacity() const {
	ScopedReadLock read(_lock);
	return _strings.capacity();
}

} // namespace core
