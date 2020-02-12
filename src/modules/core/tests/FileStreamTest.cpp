/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/io/FileStream.h"
#include "core/io/Filesystem.h"

namespace io {

class FileStreamTest : public core::AbstractTest {
};

TEST_F(FileStreamTest, testFileStreamRead) {
	const FilePtr& file = _testApp->filesystem()->open("iotest.txt");
	ASSERT_TRUE(file->exists());
	FileStream stream(file.get());
	uint8_t chr;
	uint32_t magic;
	EXPECT_EQ(0, stream.peekInt(magic));
	EXPECT_EQ(FourCC('W', 'i', 'n', 'd'), magic);
	EXPECT_EQ(0, stream.readByte(chr));
	EXPECT_EQ('W', chr);
	EXPECT_EQ(0, stream.readByte(chr));
	EXPECT_EQ('i', chr);
	EXPECT_EQ(0, stream.readByte(chr));
	EXPECT_EQ('n', chr);
	EXPECT_EQ(0, stream.peekByte(chr));
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekByte(chr));
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekByte(chr));
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.readByte(chr));
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekByte(chr));
	EXPECT_EQ('o', chr);
	char buf[8];
	stream.readString(6, buf);
	buf[6] = '\0';
	EXPECT_STREQ("owInfo", buf);
}

TEST_F(FileStreamTest, testFileStreamWrite) {
	const io::FilesystemPtr& fs = io::filesystem();
	const FilePtr& file = fs->open(fs->homePath() + "/filestream-writetest", io::FileMode::Write);
	ASSERT_TRUE(file->validHandle());
	File* fileRaw = file.get();
	FileStream stream(fileRaw);
	EXPECT_TRUE(stream.addInt(1));
	EXPECT_EQ(4l, stream.size());
	EXPECT_TRUE(stream.addInt(1));
	EXPECT_EQ(8l, stream.size());
	file->close();
	file->open(io::FileMode::Read);
	EXPECT_TRUE(file->exists());
	EXPECT_EQ(8l, file->length());
}

}
