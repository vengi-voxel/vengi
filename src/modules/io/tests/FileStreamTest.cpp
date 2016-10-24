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
	int8_t chr;
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
}

}
