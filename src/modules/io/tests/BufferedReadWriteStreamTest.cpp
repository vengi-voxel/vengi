/**
 * @file
 */

#include <gtest/gtest.h>
#include "io/BufferedReadWriteStream.h"
#include <limits.h>

namespace io {

TEST(BufferedReadWriteStreamTest, testWriteReadUInt8) {
	BufferedReadWriteStream stream;
	uint8_t writeVal = UCHAR_MAX;
	stream.writeUInt8(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	uint8_t readVal;
	EXPECT_EQ(0, stream.readUInt8(readVal));
	EXPECT_EQ(writeVal, readVal);
	EXPECT_EQ(previous - (int64_t)sizeof(readVal), stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testWriteReadUInt16) {
	BufferedReadWriteStream stream;
	uint16_t writeVal = USHRT_MAX;
	stream.writeUInt16(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	uint16_t readVal;
	EXPECT_EQ(0, stream.readUInt16(readVal));
	EXPECT_EQ(writeVal, readVal);
	EXPECT_EQ(previous - (int64_t)sizeof(readVal), stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testWriteReadUInt32) {
	BufferedReadWriteStream stream;
	uint32_t writeVal = UINT_MAX;
	stream.writeUInt32(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	uint32_t readVal;
	EXPECT_EQ(0, stream.readUInt32(readVal));
	EXPECT_EQ(writeVal, readVal);
	EXPECT_EQ(previous - (int64_t)sizeof(readVal), stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testWriteReadUInt64) {
	BufferedReadWriteStream stream;
	uint64_t writeVal = ULONG_MAX;
	stream.writeUInt64(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	uint64_t readVal;
	EXPECT_EQ(0, stream.readUInt64(readVal));
	EXPECT_EQ(writeVal, readVal);
	EXPECT_EQ(previous - (int64_t)sizeof(readVal), stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testWriteReadInt8) {
	BufferedReadWriteStream stream;
	int8_t writeVal = CHAR_MIN;
	stream.writeInt8(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	int8_t readVal;
	EXPECT_EQ(0, stream.readInt8(readVal));
	EXPECT_EQ(writeVal, readVal);
	EXPECT_EQ(previous - (int64_t)sizeof(readVal), stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testWriteReadInt16) {
	BufferedReadWriteStream stream;
	int16_t writeVal = SHRT_MIN;
	stream.writeInt16(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	int16_t readVal;
	EXPECT_EQ(0, stream.readInt16(readVal));
	EXPECT_EQ(writeVal, readVal);
	EXPECT_EQ(previous - (int64_t)sizeof(readVal), stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testWriteReadInt32) {
	BufferedReadWriteStream stream;
	int32_t writeVal = INT_MIN;
	stream.writeInt32(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	int32_t readVal;
	EXPECT_EQ(0, stream.readInt32(readVal));
	EXPECT_EQ(writeVal, readVal);
	EXPECT_EQ(previous - (int64_t)sizeof(readVal), stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testWriteReadInt64) {
	BufferedReadWriteStream stream;
	int64_t writeVal = LONG_MIN;
	stream.writeInt64(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	int64_t readVal;
	EXPECT_EQ(0, stream.readInt64(readVal));
	EXPECT_EQ(writeVal, readVal);
	EXPECT_EQ(previous - (int64_t)sizeof(readVal), stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testReadExceedsSize) {
	BufferedReadWriteStream stream;
	int8_t writeVal = 0;
	stream.writeInt8(writeVal);
	stream.seek(0);
	int64_t readVal;
	EXPECT_EQ(-1, stream.readInt64(readVal));
	EXPECT_EQ(0, stream.remaining());
}

TEST(BufferedReadWriteStreamTest, testSeek) {
	BufferedReadWriteStream stream(sizeof(uint32_t));
	EXPECT_EQ(0, stream.seek(-1));
	EXPECT_EQ(0, stream.pos());
	EXPECT_EQ(0, stream.size());
	uint32_t writeVal = 0;
	stream.writeUInt32(writeVal);
	EXPECT_EQ((int64_t)sizeof(writeVal), stream.pos());
	stream.seek(0);
	EXPECT_EQ(0, stream.pos());
}

TEST(BufferedReadWriteStreamTest, testFloat) {
	BufferedReadWriteStream stream(2 * sizeof(float));
	stream.writeFloat(1.0f);
	stream.writeFloat(-1.0f);
	EXPECT_EQ((int64_t)(2 * sizeof(float)), stream.pos());
	stream.seek(0);
	float val;
	EXPECT_EQ(0, stream.readFloat(val));
	EXPECT_FLOAT_EQ(1.0f, val);
	EXPECT_EQ(0, stream.readFloat(val));
	EXPECT_FLOAT_EQ(-1.0f, val);
	EXPECT_TRUE(stream.eos());
}

TEST(BufferedReadWriteStreamTest, testInt8) {
	BufferedReadWriteStream stream(2 * sizeof(int8_t));
	stream.writeInt8(1);
	stream.writeInt8(-1);
	EXPECT_EQ((int64_t)(2 * sizeof(int8_t)), stream.pos());
	stream.seek(0);
	int8_t val;
	EXPECT_EQ(0, stream.readInt8(val));
	EXPECT_EQ(1, val);
	EXPECT_EQ(0, stream.readInt8(val));
	EXPECT_EQ(-1, val);
	EXPECT_TRUE(stream.eos());
}

TEST(BufferedReadWriteStreamTest, testInt16) {
	BufferedReadWriteStream stream(2 * sizeof(int16_t));
	stream.writeInt16(1);
	stream.writeInt16(-1);
	EXPECT_EQ((int64_t)(2 * sizeof(int16_t)), stream.pos());
	stream.seek(0);
	int16_t val;
	EXPECT_EQ(0, stream.readInt16(val));
	EXPECT_EQ(1, val);
	EXPECT_EQ(0, stream.readInt16(val));
	EXPECT_EQ(-1, val);
	EXPECT_TRUE(stream.eos());
}

TEST(BufferedReadWriteStreamTest, testInt32) {
	BufferedReadWriteStream stream(2 * sizeof(int32_t));
	stream.writeInt32(1);
	stream.writeInt32(-1);
	EXPECT_EQ((int64_t)(2 * sizeof(int32_t)), stream.pos());
	stream.seek(0);
	int32_t val;
	EXPECT_EQ(0, stream.readInt32(val));
	EXPECT_EQ(1, val);
	EXPECT_EQ(0, stream.readInt32(val));
	EXPECT_EQ(-1, val);
	EXPECT_TRUE(stream.eos());
}

TEST(BufferedReadWriteStreamTest, testInt64) {
	BufferedReadWriteStream stream(2 * sizeof(int64_t));
	stream.writeInt64(1);
	stream.writeInt64(-1);
	EXPECT_EQ((int64_t)(2 * sizeof(int64_t)), stream.pos());
	stream.seek(0);
	int64_t val;
	EXPECT_EQ(0, stream.readInt64(val));
	EXPECT_EQ(1, val);
	EXPECT_EQ(0, stream.readInt64(val));
	EXPECT_EQ(-1, val);
	EXPECT_TRUE(stream.eos());
}

TEST(BufferedReadWriteStreamTest, testString) {
	BufferedReadWriteStream stream;
	EXPECT_TRUE(stream.writeString("foobar", true));
	stream.seek(0);
	char buf[32];
	EXPECT_TRUE(stream.readString(sizeof(buf), buf, true));
	EXPECT_STREQ("foobar", buf);
}

TEST(BufferedReadWriteStreamTest, testFormat) {
	BufferedReadWriteStream stream;
	stream.writeFormat("bsil", 1, 2, 3, 4);
	EXPECT_EQ(15, stream.pos());
	stream.seek(0);
	int8_t valB;
	int16_t valS;
	int32_t valI;
	int64_t valL;
	EXPECT_TRUE(stream.readFormat("bsil", &valB, &valS, &valI, &valL));
	EXPECT_EQ(1, valB);
	EXPECT_EQ(2, valS);
	EXPECT_EQ(3, valI);
	EXPECT_EQ(4, valL);
	EXPECT_TRUE(stream.eos());
}

}
