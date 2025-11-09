/**
 * @file
 */

#include "io/ZipArchive.h"
#include "app/tests/AbstractTest.h"
#include "core/ArrayLength.h"
#include "core/ScopedPtr.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
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
	core::ScopedPtr<io::SeekableReadStream> outstream(archive.readStream(files[0].name));
	ASSERT_TRUE(outstream);
	EXPECT_EQ(outstream->size(), (int64_t)files[0].size);
	EXPECT_EQ("file2.txt", files[0].name);
	EXPECT_EQ(25u, files[0].size);
	outstream->seek(0);
	char buf[26];
	outstream->readString(sizeof(buf), buf, false);
	buf[lengthof(buf) - 1] = '\0';
	EXPECT_STREQ("yet another file in root\n", buf);
	EXPECT_EQ("file.txt", files[1].name);
	EXPECT_EQ("file.txt", files[2].name);
	EXPECT_EQ("dir/file.txt", files[2].fullPath);
}

TEST_F(ZipArchiveTest, testZipArchiveWrite) {
	BufferedReadWriteStream archiveStream(4096);
	{
		ZipArchive archive;
		ASSERT_TRUE(archive.init(&archiveStream));

		// Write some test files
		{
			core::ScopedPtr<io::SeekableWriteStream> stream1(archive.writeStream("test1.txt"));
			ASSERT_NE(nullptr, stream1);
			const char *content1 = "Hello World!";
			stream1->writePascalStringUInt8(content1);
		}

		{
			core::ScopedPtr<io::SeekableWriteStream> stream2(archive.writeStream("test2.txt"));
			ASSERT_NE(nullptr, stream2);
			const char *content2 = "Another test file\nwith multiple lines\n";
			stream2->writePascalStringUInt8(content2);
		}

		{
			core::ScopedPtr<io::SeekableWriteStream> stream3(archive.writeStream("dir/test3.txt"));
			ASSERT_NE(nullptr, stream3);
			const char *content3 = "File in subdirectory";
			stream3->writePascalStringUInt8(content3);
		}

		archive.shutdown();
	}
	// Now read it back
	archiveStream.seek(0);
	{
		ZipArchive readArchive;
		ASSERT_TRUE(readArchive.init("test.zip", &archiveStream));

		const ArchiveFiles &files = readArchive.files();
		ASSERT_EQ(3u, files.size());

		{
			core::ScopedPtr<io::SeekableReadStream> stream(readArchive.readStream("test1.txt"));
			ASSERT_TRUE(stream);
			core::String buf;
			stream->readPascalStringUInt8(buf);
			EXPECT_EQ("Hello World!", buf);
		}

		{
			core::ScopedPtr<io::SeekableReadStream> stream(readArchive.readStream("test2.txt"));
			ASSERT_TRUE(stream);
			core::String buf;
			stream->readPascalStringUInt8(buf);
			EXPECT_EQ("Another test file\nwith multiple lines\n", buf);
		}

		{
			core::ScopedPtr<io::SeekableReadStream> stream(readArchive.readStream("dir/test3.txt"));
			ASSERT_TRUE(stream);
			core::String buf;
			stream->readPascalStringUInt8(buf);
			EXPECT_EQ("File in subdirectory", buf);
		}
	}
}

TEST_F(ZipArchiveTest, testZipArchiveWriteBinary) {
	BufferedReadWriteStream archiveStream(4096);
	{
		ZipArchive archive;
		ASSERT_TRUE(archive.init(&archiveStream));

		{
			core::ScopedPtr<io::SeekableWriteStream> stream(archive.writeStream("binary.dat"));
			ASSERT_NE(nullptr, stream);
			uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
			stream->write(data, sizeof(data));
		}

		archive.shutdown();
	}

	// Read it back
	archiveStream.seek(0);
	{
		ZipArchive readArchive;
		ASSERT_TRUE(readArchive.init("test.zip", &archiveStream));

		core::ScopedPtr<io::SeekableReadStream> stream(readArchive.readStream("binary.dat"));
		ASSERT_TRUE(stream);
		EXPECT_EQ(8, stream->size());

		uint8_t readData[8];
		EXPECT_EQ(8, stream->read(readData, 8));
		EXPECT_EQ(0x00, readData[0]);
		EXPECT_EQ(0x01, readData[1]);
		EXPECT_EQ(0x02, readData[2]);
		EXPECT_EQ(0x03, readData[3]);
		EXPECT_EQ(0xFF, readData[4]);
		EXPECT_EQ(0xFE, readData[5]);
		EXPECT_EQ(0xFD, readData[6]);
		EXPECT_EQ(0xFC, readData[7]);
	}
}

TEST_F(ZipArchiveTest, testZipArchiveWriteEmpty) {
	// Test writing an empty ZIP
	BufferedReadWriteStream archiveStream(4096);
	ZipArchive archive;
	ASSERT_TRUE(archive.init(&archiveStream));
	archive.shutdown();

	// Read it back
	archiveStream.seek(0);
	ZipArchive readArchive;
	ASSERT_TRUE(readArchive.init("test.zip", &archiveStream));

	const ArchiveFiles &files = readArchive.files();
	EXPECT_EQ(0u, files.size());
}

} // namespace io
