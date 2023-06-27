/**
 * @file
 */

#include "io/MemoryReadStream.h"
#include "core/ArrayLength.h"
#include <gtest/gtest.h>

namespace io {

class MemoryReadStreamTest : public testing::Test {};

TEST_F(MemoryReadStreamTest, testReadStream) {
	char buf[16];
	for (int i = 0; i < lengthof(buf); ++i) {
		buf[i] = (char)i;
	}
	MemoryReadStream stream(buf, sizeof(buf));
	EXPECT_EQ(stream.size(), (int64_t)sizeof(buf));
	uint8_t byte;
	for (int i = 0; i < lengthof(buf); ++i) {
		EXPECT_EQ(0, stream.readUInt8(byte));
		EXPECT_EQ((uint8_t)i, byte);
	}
	EXPECT_EQ(lengthof(buf), stream.pos());
}

TEST_F(MemoryReadStreamTest, testSeekStream) {
	const char buf[4]{0, 1, 2, 3};
	MemoryReadStream stream(buf, sizeof(buf));
	stream.seek(2);
	uint8_t byte;
	EXPECT_EQ(0, stream.readUInt8(byte));
	EXPECT_EQ(2u, byte);

	EXPECT_EQ(1u, stream.seek(-2, SEEK_CUR));
	EXPECT_EQ(0, stream.readUInt8(byte));
	EXPECT_EQ(1u, byte);
}

TEST_F(MemoryReadStreamTest, testReadString) {
	const core::String input = "name=foo\nbgcolor=bar\nvoxels=baz\n\r\n\n";
	io::MemoryReadStream stream(input.c_str(), input.size());
	core::String line;
	ASSERT_TRUE(stream.readLine(line));
	ASSERT_EQ("name=foo", line);
	ASSERT_TRUE(stream.readLine(line));
	ASSERT_EQ("bgcolor=bar", line);
	ASSERT_TRUE(stream.readLine(line));
	ASSERT_EQ("voxels=baz", line);
	ASSERT_TRUE(stream.readLine(line));
	ASSERT_EQ("", line);
	ASSERT_TRUE(stream.readLine(line));
	ASSERT_EQ("", line);
	ASSERT_FALSE(stream.readLine(line));
}

} // namespace io
