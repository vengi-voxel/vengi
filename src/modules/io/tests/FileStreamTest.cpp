/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "io/FileStream.h"

namespace io {

class FileStreamTest : public core::AbstractTest {
};

TEST_F(FileStreamTest, testFileStream) {
	const FilePtr& file = core::App::getInstance()->filesystem()->open("ui/window/testapp.tb.txt");
	ASSERT_TRUE((bool)file) << "Could not open filename " << file->getFileName();
	FileStream stream(file.get());
	uint8_t chr;
	uint32_t magic;
	ASSERT_EQ(0, stream.peekInt(magic));
	ASSERT_EQ(FourCC('W', 'i', 'n', 'd'), magic);
	ASSERT_EQ(0, stream.readByte(chr));
	ASSERT_EQ('W', chr);
	ASSERT_EQ(0, stream.readByte(chr));
	ASSERT_EQ('i', chr);
	ASSERT_EQ(0, stream.readByte(chr));
	ASSERT_EQ('n', chr);
	ASSERT_EQ(0, stream.peekByte(chr));
	ASSERT_EQ('d', chr);
	ASSERT_EQ(0, stream.peekByte(chr));
	ASSERT_EQ('d', chr);
	ASSERT_EQ(0, stream.peekByte(chr));
	ASSERT_EQ('d', chr);
	ASSERT_EQ(0, stream.readByte(chr));
	ASSERT_EQ('d', chr);
	ASSERT_EQ(0, stream.peekByte(chr));
	ASSERT_EQ('o', chr);
	char buf[8];
	stream.readString(6, buf);
	buf[6] = '\0';
	ASSERT_STREQ("owInfo", buf);
}

}
