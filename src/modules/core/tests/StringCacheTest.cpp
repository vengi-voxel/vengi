/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/StringCache.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/ReadWriteLock.h"
#include "core/concurrent/Thread.h"

namespace core {

class StringCacheTest : public testing::Test {
};

TEST_F(StringCacheTest, testEmpty) {
	StringCache cache;
	EXPECT_EQ(&String::Empty, &cache.get(""));
	EXPECT_EQ(&String::Empty, &cache.get(String::Empty));
	EXPECT_STREQ("", cache.c_str(""));
	EXPECT_EQ(0u, cache.size());
}

TEST_F(StringCacheTest, testInternStablePointer) {
	StringCache cache;
	const String &a = cache.get("undo");
	const String &b = cache.get("undo");
	EXPECT_EQ(&a, &b);
	EXPECT_EQ(a.c_str(), b.c_str());
	EXPECT_STREQ("undo", a.c_str());
	EXPECT_EQ(1u, cache.size());

	const char *c = cache.c_str("redo");
	const char *d = cache.c_str(String("redo"));
	EXPECT_EQ(c, d);
	EXPECT_STREQ("redo", c);
	EXPECT_EQ(2u, cache.size());
}

TEST_F(StringCacheTest, testGetFormat) {
	StringCache cache;
	const String &a = cache.getFormat("%s %i", "screenshot", 3);
	const String &b = cache.getFormat("%s %i", "screenshot", 3);
	EXPECT_EQ(&a, &b);
	EXPECT_STREQ("screenshot 3", a.c_str());
	EXPECT_EQ(1u, cache.size());
	EXPECT_EQ(cache.capacity(), 4096u);
}

TEST_F(StringCacheTest, testSmallCapacity) {
	StringCache cache(4);
	EXPECT_EQ(4u, cache.capacity());
	cache.get("a");
	cache.get("b");
	cache.get("c");
	cache.get("d");
	EXPECT_EQ(4u, cache.size());
}

class ReadWriteLockTest : public testing::Test {
};

TEST_F(ReadWriteLockTest, testScopedReadWrite) {
	ReadWriteLock lock;
	{
		ScopedReadLock read(lock);
		ScopedReadLock read2(lock);
	}
	{
		ScopedWriteLock write(lock);
	}
	EXPECT_TRUE(lock.tryLockRead());
	lock.unlockRead();
	EXPECT_TRUE(lock.tryLockWrite());
	lock.unlockWrite();
}

TEST_F(ReadWriteLockTest, testConcurrentReaders) {
	ReadWriteLock lock;
	AtomicInt ready(0);
	Thread t1(
		[&]() {
			ScopedReadLock read(lock);
			++ready;
		},
		"rw-reader-1");
	Thread t2(
		[&]() {
			ScopedReadLock read(lock);
			++ready;
		},
		"rw-reader-2");
	t1.join();
	t2.join();
	EXPECT_EQ(2, ready);
}

} // namespace core
