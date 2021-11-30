/**
 * @file
 */

#include <gtest/gtest.h>
#include "io/ByteStream.h"
#include <random>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <SDL_timer.h>
#include <stdlib.h>
#include "core/String.h"

namespace {
const uint8_t BYTE_ADD = UCHAR_MAX;
const int32_t INT_ADD = INT_MAX;
}

namespace io {

TEST(ByteStreamTest, testWriteInt) {
	ByteStream byteStream;
	const size_t previous = byteStream.getSize();
	byteStream.addInt(INT_ADD);
	ASSERT_EQ(previous + 4, byteStream.getSize());
}

TEST(ByteStreamTest, testReadByte) {
	ByteStream byteStream;
	byteStream.addByte(BYTE_ADD);
	const size_t previous = byteStream.getSize();
	const uint8_t byte = byteStream.readByte();
	ASSERT_EQ(BYTE_ADD, byte);
	ASSERT_EQ(previous - 1, byteStream.getSize());
}

TEST(ByteStreamTest, testReadInt) {
	ByteStream byteStream;
	byteStream.addInt(INT_ADD);
	const size_t previous = byteStream.getSize();
	int32_t dword = byteStream.readInt();
	ASSERT_EQ(INT_ADD, dword);
	ASSERT_EQ(previous - 4, byteStream.getSize());
}

}
