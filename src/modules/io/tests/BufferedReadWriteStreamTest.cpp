/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/FourCC.h"
#include "io/BufferedReadWriteStream.h"
#include <limits.h>

namespace io {

TEST(BufferedReadWriteStreamTest, testWriteReadUInt8) {
	BufferedReadWriteStream stream;
	uint8_t writeVal = UCHAR_MAX;
	stream.writeUInt8(writeVal);
	stream.seek(0);
	const int64_t previous = stream.remaining();
	uint8_t readVal = 0;
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
	uint16_t readVal = 0;
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
	uint32_t readVal = 0;
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
	uint64_t readVal = 0;
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
	int8_t readVal = 0;
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
	int16_t readVal = 0;
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
	int32_t readVal = 0;
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
	int64_t readVal = 0;
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
	float val = 0.0f;
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
	int8_t val = 0;
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
	int16_t val = 0;
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
	int32_t val = 0;
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
	int64_t val = 0;
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

TEST(BufferedReadWriteStreamTest, testPascalString8) {
	BufferedReadWriteStream stream;
	EXPECT_TRUE(stream.writePascalStringUInt8("foobar"));
	EXPECT_EQ(stream.size(), (int64_t)(sizeof(uint8_t) + 6));
	stream.writeString("ignore", true);
	stream.seek(0);
	core::String buf;
	EXPECT_TRUE(stream.readPascalStringUInt8(buf));
	EXPECT_EQ("foobar", buf);
}

TEST(BufferedReadWriteStreamTest, testPascalString16LE) {
	BufferedReadWriteStream stream;
	EXPECT_TRUE(stream.writePascalStringUInt16LE("foobar"));
	EXPECT_EQ(stream.size(), (int64_t)(sizeof(uint16_t) + 6));
	stream.writeString("ignore", true);
	stream.seek(0);
	core::String buf;
	EXPECT_TRUE(stream.readPascalStringUInt16LE(buf));
	EXPECT_EQ("foobar", buf);
}

TEST(BufferedReadWriteStreamTest, testPascalString16LEEmpty) {
	BufferedReadWriteStream stream;
	EXPECT_TRUE(stream.writePascalStringUInt16LE(""));
	EXPECT_EQ(stream.size(), (int64_t)(sizeof(uint16_t)));
	stream.writeString("ignore", true);
	stream.seek(0);
	core::String buf;
	EXPECT_TRUE(stream.readPascalStringUInt16LE(buf));
	EXPECT_EQ("", buf);
}

TEST(BufferedReadWriteStreamTest, testPascalString32LE) {
	BufferedReadWriteStream stream;
	EXPECT_TRUE(stream.writePascalStringUInt32LE("foobar"));
	EXPECT_EQ(stream.size(), (int64_t)(sizeof(uint32_t) + 6));
	stream.writeString("ignore", true);
	stream.seek(0);
	core::String buf;
	EXPECT_TRUE(stream.readPascalStringUInt32LE(buf));
	EXPECT_EQ("foobar", buf);
}

TEST(BufferedReadWriteStreamTest, testEmptyString) {
	BufferedReadWriteStream stream;
	const char *foobar = "";
	EXPECT_TRUE(stream.writeString(foobar, true));
	stream.seek(0);
	char buf[32];
	EXPECT_TRUE(stream.readString(sizeof(buf), buf, true));
	EXPECT_STREQ("", buf);
}

TEST(BufferedReadWriteStreamTest, testFormatStringTerminated) {
	BufferedReadWriteStream stream;
	const char *str = "barfoo";
	EXPECT_TRUE(stream.writeStringFormat(true, "foobar %s", str));
	stream.seek(0);
	char buf[32];
	EXPECT_TRUE(stream.readString(sizeof(buf), buf, true));
	EXPECT_STREQ("foobar barfoo", buf);
}

TEST(BufferedReadWriteStreamTest, testSkipDelta) {
	BufferedReadWriteStream stream(100 * sizeof(uint32_t));
	for (int i = 0; i < 100; ++i) {
		stream.writeUInt32(i);
	}
	EXPECT_EQ(0, stream.seek(0));
	uint32_t val;
	EXPECT_EQ(0, stream.readUInt32(val));
	EXPECT_EQ(0u, val);
	EXPECT_EQ(0, stream.skipDelta(2 * sizeof(uint32_t)));
	EXPECT_EQ(0, stream.readUInt32(val));
	EXPECT_EQ(3u, val);
	EXPECT_EQ(0, stream.skipDelta(4 * sizeof(uint32_t)));
	EXPECT_EQ(0, stream.readUInt32(val));
	EXPECT_EQ(8u, val);
	EXPECT_EQ(0, stream.skipDelta(1));
	EXPECT_EQ(0, stream.skipDelta(3));
	EXPECT_EQ(0, stream.readUInt32(val));
	EXPECT_EQ(10u, val);
}

TEST(BufferedReadWriteStreamTest, testFormat) {
	BufferedReadWriteStream stream;
	EXPECT_TRUE(stream.writeFormat("bsil", 1, 2, 3, 4));
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

TEST(BufferedReadWriteStreamTest, testFourCCLE) {
	BufferedReadWriteStream stream;
	EXPECT_TRUE(stream.writeUInt8('a'));
	EXPECT_TRUE(stream.writeUInt8('b'));
	EXPECT_TRUE(stream.writeUInt8('c'));
	EXPECT_TRUE(stream.writeUInt8('d'));
	EXPECT_NE(stream.seek(0), -1);
	const uint32_t fcc = FourCC('a', 'b', 'c', 'd');
	uint32_t fccs = 0;
	EXPECT_EQ(stream.readUInt32(fccs), 0);
	EXPECT_EQ(fcc, fccs);
}

TEST(BufferedReadWriteStreamTest, testFourCCBE) {
	BufferedReadWriteStream stream;
	EXPECT_TRUE(stream.writeUInt8('d'));
	EXPECT_TRUE(stream.writeUInt8('c'));
	EXPECT_TRUE(stream.writeUInt8('b'));
	EXPECT_TRUE(stream.writeUInt8('a'));
	EXPECT_NE(stream.seek(0), -1);
	const uint32_t fcc = FourCC('a', 'b', 'c', 'd');
	uint32_t fccs = 0;
	EXPECT_EQ(stream.readUInt32BE(fccs), 0);
	EXPECT_EQ(fcc, fccs);
}

TEST(BufferedReadWriteStreamTest, testUTF16) {
	BufferedReadWriteStream stream;
	core::String str = "foobar string %&";
	EXPECT_TRUE(stream.writeUTF16BE(str));
	const int64_t utf16Len = stream.pos();
	EXPECT_EQ(utf16Len, (int64_t)(str.size() * sizeof(uint16_t)));
	stream.seek(0);
	core::String str2;
	EXPECT_TRUE(stream.readUTF16BE(str.size(), str2));
	EXPECT_STREQ(str.c_str(), str2.c_str());
}

}
