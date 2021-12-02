/**
 * @file
 */

#include <gtest/gtest.h>
#include "io/BufferedReadWriteStream.h"
#include <random>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <SDL_timer.h>
#include <stdlib.h>
#include "core/String.h"

namespace {
const uint8_t BYTE_ADD = UCHAR_MAX;
const uint32_t INT_ADD = INT_MAX;
}

namespace io {

TEST(BufferedReadWriteStreamTest, testWriteInt) {
	BufferedReadWriteStream stream;
	const int64_t previous = stream.size();
	stream.writeInt(INT_ADD);
	ASSERT_EQ(previous + 4, stream.size());
}

TEST(BufferedReadWriteStreamTest, testReadByte) {
	BufferedReadWriteStream stream;
	stream.writeByte(BYTE_ADD);
	const int64_t previous = stream.remaining();
	uint8_t byte;
	EXPECT_EQ(0, stream.readByte(byte));
	ASSERT_EQ(BYTE_ADD, byte);
	ASSERT_EQ(previous - 1, stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testReadInt) {
	BufferedReadWriteStream stream;
	stream.writeInt(INT_ADD);
	const int64_t previous = stream.remaining();
	uint32_t dword;
	EXPECT_EQ(0, stream.readInt(dword));
	ASSERT_EQ(INT_ADD, dword);
	ASSERT_EQ(previous - 4, stream.remaining());
}

}
