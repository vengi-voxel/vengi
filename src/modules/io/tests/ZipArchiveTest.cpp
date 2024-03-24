/**
 * @file
 */

#include "io/ZipArchive.h"
#include "app/tests/AbstractTest.h"
#include "core/ArrayLength.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include <gtest/gtest.h>

namespace io {

class ZipArchiveTest : public app::AbstractTest {};

TEST_F(ZipArchiveTest, testZipArchive) {
	const io::FilePtr &file = _testApp->filesystem()->open("iotest.zip", io::FileMode::Read);
	FileStream fileStream(file);
	ZipArchive archive;
	ASSERT_TRUE(archive.init(file->fileName(), &fileStream));
	const ArchiveFiles &files = archive.files();
	ASSERT_EQ(3u, files.size());
	BufferedReadWriteStream outstream((int64_t)files[0].size);
	archive.load(files[0].name, outstream);
	EXPECT_EQ(outstream.size(), (int64_t)files[0].size);
	EXPECT_EQ("file2.txt", files[0].name);
	EXPECT_EQ(25u, files[0].size);
	outstream.seek(0);
	char buf[26];
	outstream.readString(sizeof(buf), buf, false);
	buf[lengthof(buf) - 1] = '\0';
	EXPECT_STREQ("yet another file in root\n", buf);
	EXPECT_EQ("file.txt", files[1].name);
	EXPECT_EQ("file.txt", files[2].name);
	EXPECT_EQ("dir/file.txt", files[2].fullPath);
}

TEST_F(ZipArchiveTest, testLoadToFile) {
	const io::FilePtr &file = _testApp->filesystem()->open("iotest.zip", io::FileMode::Read);
	FileStream fileStream(file);
	ZipArchive archive;
	ASSERT_TRUE(archive.init(file->fileName(), &fileStream));
	const ArchiveFiles &files = archive.files();
	ASSERT_EQ(3u, files.size());

	int fileIdx = 2;
	const io::FilePtr &targetFile = _testApp->filesystem()->open(files[fileIdx].fullPath, io::FileMode::Write);
	io::FileStream targetStream(targetFile);
	ASSERT_TRUE(archive.load(files[fileIdx].fullPath, targetStream));
}

} // namespace io
