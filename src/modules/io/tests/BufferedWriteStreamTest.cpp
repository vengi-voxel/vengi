/**
 * @file
 */

#include "io/BufferedWriteStream.h"
#include "io/BufferedReadWriteStream.h"
#include <limits.h>
#include <gtest/gtest.h>

namespace io {

TEST(BufferedWriteStreamTest, testWriteReadUInt8) {
	BufferedReadWriteStream child;
	// here we are testing the flush and realloc - so make this buffer small enough
	BufferedWriteStream stream(child, 32);

	uint8_t writeVal = UCHAR_MAX;
	stream.writeUInt8(writeVal);

	EXPECT_EQ(0u, child.size()) << "Unexpected write to child stream";
	EXPECT_TRUE(stream.flush()) << "Failed to flush";
	EXPECT_EQ(1u, child.size()) << "Flush didn't write to child stream";

	stream.writeUInt32(1);
	EXPECT_EQ(1u, child.size()) << "Unexpected write to child stream";
	stream.writeUInt32(1);
	EXPECT_EQ(1u, child.size()) << "Unexpected write to child stream";
	stream.writeUInt32(1);
	EXPECT_EQ(1u, child.size()) << "Unexpected write to child stream";
	stream.writeUInt32(1);
	EXPECT_EQ(1u, child.size()) << "Unexpected write to child stream";
	stream.writeUInt32(1);
	EXPECT_EQ(1u, child.size()) << "Unexpected write to child stream";
	stream.writeUInt32(1);
	EXPECT_EQ(1u, child.size()) << "Unexpected write to child stream";
	stream.writeUInt32(1);
	EXPECT_EQ(1u, child.size()) << "Unexpected write to child stream";
	stream.writeUInt32(1);
	EXPECT_EQ(1u, child.size()) << "Expected write to child stream";
	stream.writeUInt32(1);
	EXPECT_EQ(33u, child.size()) << "Unexpected write to child stream";
	stream.flush();
	EXPECT_EQ(37u, child.size()) << "Unexpected write to child stream";
}

} // namespace io
