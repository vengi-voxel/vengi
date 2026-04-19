/**
 * @file
 */

#include "io/CachingArchive.h"
#include "core/ScopedPtr.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include <gtest/gtest.h>

namespace io {

class CachingArchiveTest : public testing::Test {};

TEST_F(CachingArchiveTest, testFindStreamDirect) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {10, 20, 30};
	mem->add("parts/config.ldr", buf, sizeof(buf));

	CachingArchive cache(mem);
	cache.registerSearchDir("parts", "*.ldr");
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("config.ldr"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(3, stream->size());
}

TEST_F(CachingArchiveTest, testFindStreamCaseInsensitive) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {10, 20, 30};
	mem->add("parts/ldconfig.ldr", buf, sizeof(buf));

	CachingArchive cache(mem);
	cache.registerSearchDir("parts", "*.ldr");
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("LDConfig.LdR"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(3, stream->size());
}

TEST_F(CachingArchiveTest, testFindStreamNotFound) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {10, 20, 30};
	mem->add("parts/brick.dat", buf, sizeof(buf));

	CachingArchive cache(mem);
	// no search dirs registered
	EXPECT_FALSE(cache.findStream("brick.dat"));
}

TEST_F(CachingArchiveTest, testFindStreamViaSearchDir) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {10, 20, 30};
	mem->add("lib/parts/brick.dat", buf, sizeof(buf));

	CachingArchive cache(mem);
	cache.registerSearchDir("lib/parts", "*.dat");
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("brick.dat"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(3, stream->size());
}

TEST_F(CachingArchiveTest, testFindStreamFilterApplied) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {10, 20, 30};
	mem->add("parts/brick.dat", buf, sizeof(buf));
	mem->add("parts/readme.txt", buf, sizeof(buf));

	CachingArchive cache(mem);
	cache.registerSearchDir("parts", "*.dat");
	// .dat file should be found
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("brick.dat"));
	EXPECT_TRUE(stream);
	// .txt file should not be cached because it doesn't match the filter
	EXPECT_FALSE(cache.findStream("readme.txt"));
}

TEST_F(CachingArchiveTest, testMultipleSearchDirs) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf1[] = {1};
	uint8_t buf2[] = {2, 3};
	mem->add("parts/brick.dat", buf1, sizeof(buf1));
	mem->add("p/prim.dat", buf2, sizeof(buf2));

	CachingArchive cache(mem);
	cache.registerSearchDir("parts", "*.dat");
	cache.registerSearchDir("p", "*.dat");

	core::ScopedPtr<SeekableReadStream> s1(cache.findStream("brick.dat"));
	ASSERT_TRUE(s1);
	EXPECT_EQ(1, s1->size());

	core::ScopedPtr<SeekableReadStream> s2(cache.findStream("prim.dat"));
	ASSERT_TRUE(s2);
	EXPECT_EQ(2, s2->size());
}

TEST_F(CachingArchiveTest, testFirstRegisteredWins) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf1[] = {1};
	uint8_t buf2[] = {2, 3};
	mem->add("dir1/file.dat", buf1, sizeof(buf1));
	mem->add("dir2/file.dat", buf2, sizeof(buf2));

	CachingArchive cache(mem);
	cache.registerSearchDir("dir1", "*.dat");
	cache.registerSearchDir("dir2", "*.dat");

	// first registered dir wins
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("file.dat"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(1, stream->size());
}

} // namespace io
