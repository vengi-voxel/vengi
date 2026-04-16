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

TEST_F(CachingArchiveTest, testExistsFromCache) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {1, 2, 3};
	mem->add("file.txt", buf, sizeof(buf));

	CachingArchive cache(mem);
	EXPECT_TRUE(cache.exists("file.txt"));
	EXPECT_FALSE(cache.exists("missing.txt"));
}

TEST_F(CachingArchiveTest, testListFromCache) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {1, 2, 3};
	mem->add("a.vox", buf, sizeof(buf));
	mem->add("b.txt", buf, sizeof(buf));
	mem->add("c.vox", buf, sizeof(buf));

	CachingArchive cache(mem);
	ArchiveFiles files;
	cache.list("", files, "*.vox");
	ASSERT_EQ(2u, files.size());
}

TEST_F(CachingArchiveTest, testReadStreamForwarded) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {10, 20, 30};
	mem->add("data.bin", buf, sizeof(buf));

	CachingArchive cache(mem);
	core::ScopedPtr<SeekableReadStream> stream(cache.readStream("data.bin"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(3, stream->size());
}

TEST_F(CachingArchiveTest, testWriteInvalidatesCache) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {1, 2, 3};
	mem->add("file.txt", buf, sizeof(buf));

	CachingArchive cache(mem);
	// populate cache
	EXPECT_TRUE(cache.exists("file.txt"));
	EXPECT_FALSE(cache.exists("new.txt"));

	// write new file via cache
	{
		core::ScopedPtr<SeekableWriteStream> ws(cache.writeStream("new.txt"));
		ASSERT_TRUE(ws);
		ws->write(buf, sizeof(buf));
	}

	// cache should be invalidated - new file should be found
	EXPECT_TRUE(cache.exists("new.txt"));
}

TEST_F(CachingArchiveTest, testPathNormalization) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {1, 2, 3};
	mem->add("dir/file.vox", buf, sizeof(buf));

	CachingArchive cache(mem);
	EXPECT_TRUE(cache.exists("./dir/file.vox"));
	EXPECT_TRUE(cache.exists("other/../dir/file.vox"));
}

TEST_F(CachingArchiveTest, testInvalidate) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {1, 2, 3};
	mem->add("file.txt", buf, sizeof(buf));

	CachingArchive cache(mem);
	EXPECT_TRUE(cache.exists("file.txt"));

	// add directly to underlying archive
	mem->add("sneaky.txt", buf, sizeof(buf));

	// cache still stale
	EXPECT_FALSE(cache.exists("sneaky.txt"));

	// explicitly invalidate
	cache.invalidate();
	EXPECT_TRUE(cache.exists("sneaky.txt"));
}

} // namespace io
