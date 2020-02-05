/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/io/Filesystem.h"
#include "core/Enum.h"
#include <algorithm>

namespace io {

class FilesystemTest: public core::AbstractTest {
};

::std::ostream& operator<<(::std::ostream& ostream, const std::vector<io::Filesystem::DirEntry>& val) {
	for (const auto& e : val) {
		ostream << e.name << " - " << core::enumVal(e.type) << ", ";
	}
	return ostream;
}

TEST_F(FilesystemTest, testListDirectory) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.createDir("listdirtest/dir1"));
	EXPECT_TRUE(fs.syswrite("listdirtest/dir1/ignored", "ignore"));
	EXPECT_TRUE(fs.syswrite("listdirtest/dir1/ignoredtoo", "ignore"));
	EXPECT_TRUE(fs.syswrite("listdirtest/file1", "1"));
	EXPECT_TRUE(fs.syswrite("listdirtest/file2", "2"));
	std::vector<io::Filesystem::DirEntry> entities;
	fs.list("listdirtest/", entities, "");
	EXPECT_FALSE(entities.empty());
	EXPECT_EQ(3u, entities.size()) << entities;
	std::sort(entities.begin(), entities.end(),
		[] (const io::Filesystem::DirEntry& first, const io::Filesystem::DirEntry& second) {
			return first.name < second.name;
		});
	EXPECT_EQ("dir1", entities[0].name);
	EXPECT_EQ("file1", entities[1].name);
	EXPECT_EQ("file2", entities[2].name);
	EXPECT_EQ(io::Filesystem::DirEntry::Type::dir, entities[0].type);
	EXPECT_EQ(io::Filesystem::DirEntry::Type::file, entities[1].type);
	EXPECT_EQ(io::Filesystem::DirEntry::Type::file, entities[2].type);
	fs.shutdown();
}

TEST_F(FilesystemTest, testAbsolutePath) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.createDir("absolutePathInCurDir"));
	const core::String& absPath = fs.absolutePath("absolutePathInCurDir");
	EXPECT_NE("", absPath);
	fs.shutdown();
}

TEST_F(FilesystemTest, testIsRelativePath) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.isRelativePath("./foo"));
	EXPECT_TRUE(fs.isRelativePath("foo"));
	EXPECT_TRUE(fs.isRelativePath("foo/bar"));
	EXPECT_TRUE(fs.isRelativePath("foo/bar/"));
	EXPECT_FALSE(fs.isRelativePath("/foo"));
	EXPECT_FALSE(fs.isRelativePath("/foo/bar"));
	EXPECT_FALSE(fs.isRelativePath("/foo/bar/"));
	fs.shutdown();
}

TEST_F(FilesystemTest, testIsReadableDir) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.isReadableDir(fs.homePath())) << fs.homePath() << " is not readable";
	fs.shutdown();
}

TEST_F(FilesystemTest, testListFilter) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.createDir("listdirtestfilter"));
	EXPECT_TRUE(fs.createDir("listdirtestfilter/dirxyz"));
	EXPECT_TRUE(fs.syswrite("listdirtestfilter/filexyz", "1"));
	EXPECT_TRUE(fs.syswrite("listdirtestfilter/fileother", "2"));
	EXPECT_TRUE(fs.syswrite("listdirtestfilter/fileignore", "3"));
	std::vector<io::Filesystem::DirEntry> entities;
	fs.list("listdirtestfilter/", entities, "*xyz");
	EXPECT_EQ(2u, entities.size()) << entities;
	EXPECT_EQ(io::Filesystem::DirEntry::Type::dir, entities[0].type);
	EXPECT_EQ(io::Filesystem::DirEntry::Type::file, entities[1].type);
	fs.shutdown();
}

TEST_F(FilesystemTest, testMkdir) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.createDir("testdir"));
	EXPECT_TRUE(fs.createDir("testdir2/subdir/other"));
	EXPECT_TRUE(fs.removeDir("testdir2/subdir/other"));
	EXPECT_TRUE(fs.removeDir("testdir2/subdir"));
	EXPECT_TRUE(fs.removeDir("testdir2"));
	fs.shutdown();
}

TEST_F(FilesystemTest, testWriteExplicitCurDir) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.write("./testfile", "123")) << "Failed to write content to testfile in " << fs.homePath();
	const core::String& content = fs.load("./testfile");
	EXPECT_EQ("123", content) << "Written content doesn't match expected";
	fs.shutdown();
}

TEST_F(FilesystemTest, testWrite) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.write("testfile", "123")) << "Failed to write content to testfile in " << fs.homePath();
	const core::String& content = fs.load("testfile");
	EXPECT_EQ("123", content) << "Written content doesn't match expected";
	fs.shutdown();
}

TEST_F(FilesystemTest, testWriteNewDir) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.write("dir123/testfile", "123")) << "Failed to write content to testfile in dir123";
	const core::String& content = fs.load("dir123/testfile");
	EXPECT_EQ("123", content) << "Written content doesn't match expected";
	fs.removeFile("dir123/testfile");
	fs.removeDir("dir123");
	fs.shutdown();
}

TEST_F(FilesystemTest, testCreateDirRecursive) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_TRUE(fs.createDir("dir1/dir2/dir3/dir4", true));
	EXPECT_TRUE(fs.removeDir("dir1/dir2/dir3/dir4"));
	EXPECT_TRUE(fs.removeDir("dir1/dir2/dir3"));
	EXPECT_TRUE(fs.removeDir("dir1/dir2"));
	EXPECT_TRUE(fs.removeDir("dir1"));
	fs.shutdown();
}

TEST_F(FilesystemTest, testCreateDirNonRecursiveFail) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	EXPECT_FALSE(fs.createDir("does/not/exist", false));
	fs.shutdown();
}

}
