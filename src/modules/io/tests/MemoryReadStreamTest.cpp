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

TEST_F(MemoryReadStreamTest, testEOS) {
	const char buf[4]{0, 1, 2, 3};
	MemoryReadStream stream(buf, sizeof(buf));
	stream.skip(4);
	EXPECT_TRUE(stream.eos());
}

TEST_F(MemoryReadStreamTest, testReadString) {
	const core::String input = "name=foo\n";
	io::MemoryReadStream stream(input.c_str(), input.size());
	core::String str;
	ASSERT_TRUE(stream.readString(9, str, true));
	ASSERT_EQ("name=foo\n", str);
	ASSERT_TRUE(stream.eos());
	stream.seek(0, SEEK_SET);
	ASSERT_TRUE(stream.readString(4, str, false));
	ASSERT_EQ("name", str);
	ASSERT_FALSE(stream.eos());
	ASSERT_TRUE(stream.readString(3, str, false));
	ASSERT_EQ("=fo", str);
	ASSERT_FALSE(stream.eos());
}

} // namespace io
