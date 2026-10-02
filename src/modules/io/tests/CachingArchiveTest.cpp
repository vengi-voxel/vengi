/**
 * @file
 */

#include "io/CachingArchive.h"
#include "core/ScopedPtr.h"
#include "core/SharedPtr.h"
#include "io/Archive.h"
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

TEST_F(CachingArchiveTest, testFindStreamSubdirPath) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {10, 20, 30};
	mem->add("p/48/1-12cylo.dat", buf, sizeof(buf));

	CachingArchive cache(mem);
	cache.registerSearchDir("p", "*.dat");
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("48/1-12cylo.dat"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(3, stream->size());

	core::ScopedPtr<SeekableReadStream> streamBs(cache.findStream("48\\1-12cylo.dat"));
	ASSERT_TRUE(streamBs);
	EXPECT_EQ(3, streamBs->size());
}

TEST_F(CachingArchiveTest, testFindStreamRelativeCurrentDir) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {10, 20, 30};
	mem->add("./hmec.hva", buf, sizeof(buf));

	CachingArchive cache(mem);
	cache.registerSearchDir("./", "*.hva");
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("hmec.hva"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(3, stream->size());
}

TEST_F(CachingArchiveTest, testFindStreamEmptyDir) {
	MemoryArchivePtr mem = openMemoryArchive();
	uint8_t buf[] = {1, 2};
	mem->add("test.vxl", buf, sizeof(buf));

	CachingArchive cache(mem);
	cache.registerSearchDir("", "*.vxl");
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("test.vxl"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(2, stream->size());
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

// Simulates FilesystemArchive: list() is scoped to a relative dir, but fullPath is absolute.
class AbsolutePathArchive : public Archive {
private:
	MemoryArchivePtr _mem;
	core::String _relDir;
	core::String _absDir;

public:
	AbsolutePathArchive(const core::String &relDir, const core::String &absDir)
		: _mem(openMemoryArchive()), _relDir(relDir), _absDir(absDir) {
	}

	void add(const core::String &fileName, const uint8_t *data, size_t size) {
		_mem->add(_absDir + "/" + fileName, data, size);
	}

	bool exists(const core::String &file) const override {
		return _mem->exists(file);
	}

	void list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const override {
		if (!basePath.empty() && basePath != _relDir && basePath != _relDir + "/") {
			return;
		}
		ArchiveFiles absFiles;
		_mem->list(_absDir, absFiles, filter);
		for (FilesystemEntry entry : absFiles) {
			const size_t slash = entry.fullPath.rfind('/');
			entry.name = slash == core::String::npos ? entry.fullPath : entry.fullPath.substr(slash + 1);
			out.push_back(entry);
		}
	}

	SeekableReadStream *readStream(const core::String &filePath) override {
		return _mem->readStream(filePath);
	}

	SeekableWriteStream *writeStream(const core::String &filePath) override {
		return _mem->writeStream(filePath);
	}
};

TEST_F(CachingArchiveTest, testAbsoluteFullPathRelativeSearchDir) {
	uint8_t buf[] = {10, 20, 30};
	auto archive = core::make_shared<AbsolutePathArchive>("bug636", "/home/user/project/bug636");
	archive->add("legs.hva", buf, sizeof(buf));

	CachingArchive cache(archive);
	cache.registerSearchDir("bug636/", "*.hva");
	EXPECT_TRUE(cache.exists("bug636/legs.hva"));
	EXPECT_TRUE(cache.exists("legs.hva"));
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("bug636/legs.hva"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(3, stream->size());
}

// FilesystemArchive on Windows yields drive-letter fullPath values (often with backslashes).
// sanitizePath/lexicallyNormal converts those to C:/... so the '/' needle still matches.
TEST_F(CachingArchiveTest, testAbsoluteFullPathRelativeSearchDirWindows) {
	uint8_t buf[] = {10, 20, 30};
	auto archive = core::make_shared<AbsolutePathArchive>("bug636", "C:\\Users\\foo\\project\\bug636");
	archive->add("legs.hva", buf, sizeof(buf));

	CachingArchive cache(archive);
	cache.registerSearchDir("bug636/", "*.hva");
	EXPECT_TRUE(cache.exists("bug636/legs.hva"));
	EXPECT_TRUE(cache.exists("bug636\\legs.hva"));
	EXPECT_TRUE(cache.exists("legs.hva"));
	core::ScopedPtr<SeekableReadStream> stream(cache.findStream("bug636/legs.hva"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(3, stream->size());
}

} // namespace io
