/**
 * @file
 */

#include <gtest/gtest.h>
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "core/FourCC.h"

namespace io {

class FileStreamTest : public testing::Test {
};

TEST_F(FileStreamTest, testFileStreamRead) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	const FilePtr& file = fs.open("iotest.txt");
	ASSERT_TRUE(file->exists());
	FileStream stream(file.get());
	const int64_t remaining = stream.remaining();
	EXPECT_EQ((int64_t)file->length(), remaining);
	uint8_t chr;
	uint32_t magic;
	EXPECT_EQ(0, stream.peekInt(magic));
	EXPECT_EQ(remaining, stream.remaining());
	EXPECT_EQ(FourCC('W', 'i', 'n', 'd'), magic);
	EXPECT_EQ(0, stream.readByte(chr));
	EXPECT_EQ(remaining, stream.remaining() + 1);
	EXPECT_EQ('W', chr);
	EXPECT_EQ(0, stream.readByte(chr));
	EXPECT_EQ(remaining, stream.remaining() + 2);
	EXPECT_EQ('i', chr);
	EXPECT_EQ(0, stream.readByte(chr));
	EXPECT_EQ(remaining, stream.remaining() + 3);
	EXPECT_EQ('n', chr);
	EXPECT_EQ(0, stream.peekByte(chr));
	EXPECT_EQ(remaining, stream.remaining() + 3);
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekByte(chr));
	EXPECT_EQ(remaining, stream.remaining() + 3);
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekByte(chr));
	EXPECT_EQ(remaining, stream.remaining() + 3);
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.readByte(chr));
	EXPECT_EQ(remaining, stream.remaining() + 4);
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekByte(chr));
	EXPECT_EQ(remaining, stream.remaining() + 4);
	EXPECT_EQ('o', chr);
	char buf[8];
	stream.readString(6, buf);
	EXPECT_EQ(remaining, stream.remaining() + 10);
	buf[6] = '\0';
	EXPECT_STREQ("owInfo", buf);
}

TEST_F(FileStreamTest, testFileStreamWrite) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	const FilePtr& file = fs.open("filestream-writetest", io::FileMode::SysWrite);
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
