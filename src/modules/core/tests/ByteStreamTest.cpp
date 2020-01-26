/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/ByteStream.h"
#include <random>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <SDL_timer.h>
#include <stdlib.h>
#include "core/String.h"

namespace {
const uint8_t BYTE_ADD = UCHAR_MAX;
const int16_t SHORT_ADD = SHRT_MAX;
const int32_t INT_ADD = INT_MAX;
}

namespace core {

TEST(ByteStreamTest, testCopy) {
	std::vector<ByteStream> v;
	for (int j = 0; j < 1000; ++j) {
		ByteStream byteStream;
		for (int i = 0; i < 1000; ++i) {
			byteStream.addInt(i);
		}
		byteStream.addByte(1, true);
		v.push_back(byteStream);
	}

	for (std::vector<ByteStream>::const_iterator i = v.begin(); i != v.end(); ++i) {
		ByteStream s = *i;
		ASSERT_EQ(4001u, s.getSize());
	}

	ASSERT_EQ(1000u, v.size());
}

TEST(ByteStreamTest, DISABLED_testBigChunk) {
	const int size = 1000 * 1000 * 50;
	core::ByteStream bs(size);
	for (int i = 0; i < size; ++i) {
		bs.addByte(1);
	}

	for (int i = 0; i < size; ++i) {
		bs.readByte();
	}
}

TEST(ByteStreamTest, DISABLED_testBigChunkAdd) {
	const int size = 1000 * 1000 * 50;
	core::ByteStream bs(size);
	for (int i = 0; i < size; ++i) {
		bs.addByte(1);
	}
}

TEST(ByteStreamTest, testFormat) {
	core::ByteStream bs;
	bs.addFormat("ibli", 245678, 1, 2L, 12345678);
	ASSERT_EQ(17u, bs.getSize());
	int len;
	int version;
	long seed;
	int size;
	bs.readFormat("ibli", &len, &version, &seed, &size);
	ASSERT_EQ(245678, len);
	ASSERT_EQ(1, version);
	ASSERT_EQ(2L, seed);
	ASSERT_EQ(12345678, size);
}

TEST(ByteStreamTest, testWriteByte) {
	ByteStream byteStream;
	const size_t previous = byteStream.getSize();
	byteStream.addByte(BYTE_ADD);
	ASSERT_EQ(previous + 1, byteStream.getSize());
}

TEST(ByteStreamTest, testWriteShort) {
	ByteStream byteStream;
	const size_t previous = byteStream.getSize();
	byteStream.addShort(SHORT_ADD);
	ASSERT_EQ(previous + 2, byteStream.getSize());
}

TEST(ByteStreamTest, testWriteEmptyString) {
	ByteStream byteStream;
	byteStream.addString("");
	ASSERT_EQ(1u, byteStream.getSize());
	const core::String empty = byteStream.readString();
	ASSERT_EQ("", empty);
	ASSERT_EQ(0u, byteStream.getSize());
}

TEST(ByteStreamTest, testWriteInt) {
	ByteStream byteStream;
	const size_t previous = byteStream.getSize();
	byteStream.addInt(INT_ADD);
	ASSERT_EQ(previous + 4, byteStream.getSize());
}

TEST(ByteStreamTest, testWriteLong) {
	ByteStream byteStream;
	const size_t previous = byteStream.getSize();
	byteStream.addLong(234L);
	ASSERT_EQ(previous + 8, byteStream.getSize());
}

TEST(ByteStreamTest, testPeekShort) {
	int16_t peek;
	ByteStream byteStream;
	peek = byteStream.peekShort();
	byteStream.addByte(1);
	ASSERT_EQ(-1, peek);
	byteStream.addByte(1);
	peek = byteStream.peekShort();
	ASSERT_EQ(257, peek);
	ASSERT_EQ(257, byteStream.readShort());
	peek = byteStream.peekShort();
	ASSERT_EQ(-1, peek);
}

TEST(ByteStreamTest, testReadByte) {
	ByteStream byteStream;
	byteStream.addByte(BYTE_ADD);
	const size_t previous = byteStream.getSize();
	const uint8_t byte = byteStream.readByte();
	ASSERT_EQ(BYTE_ADD, byte);
	ASSERT_EQ(previous - 1, byteStream.getSize());
}

TEST(ByteStreamTest, testReadShort) {
	ByteStream byteStream;
	byteStream.addShort(SHORT_ADD);
	const size_t previous = byteStream.getSize();
	const int16_t word = byteStream.readShort();
	ASSERT_EQ(SHORT_ADD, word);
	ASSERT_EQ(previous - 2, byteStream.getSize());
}

TEST(ByteStreamTest, testReadInt) {
	ByteStream byteStream;
	byteStream.addInt(INT_ADD);
	const size_t previous = byteStream.getSize();
	int32_t dword = byteStream.readInt();
	ASSERT_EQ(INT_ADD, dword);
	ASSERT_EQ(previous - 4, byteStream.getSize());
}

TEST(ByteStreamTest, testReadLong) {
	ByteStream byteStream;
	byteStream.addLong(234L);
	const size_t previous = byteStream.getSize();
	int64_t dword = byteStream.readLong();
	ASSERT_EQ(234L, dword);
	ASSERT_EQ(previous - 8, byteStream.getSize());
}

TEST(ByteStreamTest, testReadFloat) {
	ByteStream byteStream;
	const float expected = 0.1f;
	byteStream.addFloat(expected);
	const size_t previous = byteStream.getSize();
	const float dword = byteStream.readFloat();
	ASSERT_DOUBLE_EQ(expected, dword);
	ASSERT_EQ(previous - 4, byteStream.getSize());
}

TEST(ByteStreamTest, testReadString) {
	ByteStream byteStream;
	const core::String str = "hello IT!";
	byteStream.addString(str);
	const size_t previous = byteStream.getSize();
	EXPECT_EQ(str.size(), previous - 1);
	const core::String readstr = byteStream.readString();
	ASSERT_EQ(str, readstr);
	ASSERT_EQ(previous - str.size() - 1, byteStream.getSize());
}

TEST(ByteStreamTest, testReadStrings) {
	ByteStream byteStream;
	byteStream.addString("hello IT!");
	byteStream.addString("some other string");
	byteStream.addString("yet another string");
	byteStream.addString("0");
	byteStream.readString();
	byteStream.readString();
	byteStream.readString();
	ASSERT_EQ("0", byteStream.readString());
}

TEST(ByteStreamTest, testReadWriteVariadic) {
	ByteStream byteStream;
	const uint8_t byte = BYTE_ADD;
	const int16_t word = SHORT_ADD;
	const int32_t dword = INT_ADD;
	int pByte;
	int pShort;
	int pInt;
	byteStream.addFormat("bsi", byte, word, dword);
	byteStream.readFormat("bsi", &pByte, &pShort, &pInt);
	ASSERT_TRUE(byte == pByte);
	ASSERT_TRUE(word == pShort);
	ASSERT_TRUE(dword == pInt);
}

TEST(ByteStreamTest, testReadWriteAll) {
	ByteStream byteStream;
	std::default_random_engine engine;
	std::uniform_int_distribution<int> distribution(0, RAND_MAX);
	uint8_t byte = distribution(engine) % BYTE_ADD;
	int16_t word = distribution(engine) % SHORT_ADD;
	int32_t dword = distribution(engine) % INT_ADD;
	float floatv = floorf((distribution(engine) % INT_ADD) / float(INT_ADD) * 100.0)
			/ 100.0;
	size_t size = sizeof(byte) + sizeof(word) + sizeof(dword) + sizeof(floatv);
	byteStream.addByte(byte);
	byteStream.addShort(word);
	byteStream.addInt(dword);
	byteStream.addFloat(floatv);
	ASSERT_EQ(byteStream.getSize(), size);
	ASSERT_EQ(byteStream.readByte(), byte);
	ASSERT_EQ(byteStream.getSize(), size -= 1);
	ASSERT_EQ(byteStream.readShort(), word);
	ASSERT_EQ(byteStream.getSize(), size -= 2);
	ASSERT_EQ(byteStream.readInt(), dword);
	ASSERT_EQ(byteStream.getSize(), size -= 4);
	ASSERT_DOUBLE_EQ(byteStream.readFloat(), floatv);
	ASSERT_EQ(byteStream.getSize(), size_t(0));
}

typedef enum DataType {
	e_byte = 0, e_short, e_int, e_float, e_string, count
} DataType;

typedef struct TypeValue {
	DataType type;
	void* pValue;
} TypeValue;

typedef std::vector<TypeValue> TypeValueList;
typedef TypeValueList::iterator TypeValueListIter;

TEST(ByteStreamTest, testRandomReadWrite) {
	ByteStream byteStream;
	std::default_random_engine engine;
	std::uniform_int_distribution<int> distribution(0, RAND_MAX);

	TypeValueList _typeValueList;

	unsigned int iterations = distribution(engine) % 20 + 1;
	unsigned int index = 0;
	size_t size = 0;

	//add random types to byte stream
	do {
		DataType dataType = DataType(distribution(engine) % count);
		TypeValue typeValue;
		typeValue.type = dataType;
		switch (dataType) {
		case e_byte: {
			uint8_t* byte = new uint8_t(distribution(engine) % BYTE_ADD);
			byteStream.addByte(*byte);
			typeValue.pValue = byte;
			size += 1;
			break;
		}
		case e_short: {
			int16_t* word = new int16_t(distribution(engine) % SHORT_ADD);
			byteStream.addShort(*word);
			typeValue.pValue = word;
			size += 2;
			break;
		}
		case e_int: {
			int32_t* dword = new int32_t(distribution(engine) % INT_ADD);
			byteStream.addInt(*dword);
			typeValue.pValue = dword;
			size += 4;
			break;
		}
		case e_float: {
			float* dword = new float(
					floorf((distribution(engine) % INT_ADD) / float(INT_ADD) * 100.0)
							/ 100.0);
			byteStream.addFloat(*dword);
			typeValue.pValue = dword;
			size += 4;
			break;
		}
		case e_string: {
			core::String* str = new core::String("hello IT!");
			byteStream.addString(*str);
			typeValue.pValue = str;
			size += str->size() + 1; //plus the '\0' char
			break;
		}
		default:
			ASSERT_TRUE(false);
			break;
		}
		_typeValueList.push_back(typeValue);
	} while (index++ < iterations);
	ASSERT_EQ(byteStream.getSize(), size);

	//read and verify added types in byte stream
	index = 0;
	do {
		DataType dataType = _typeValueList.front().type;
		void* value = _typeValueList.front().pValue;
		switch (dataType) {
		case e_byte: {
			uint8_t byte = byteStream.readByte();
			size -= 1;
			ASSERT_EQ(byte, *(uint8_t* ) value);
			delete (uint8_t*) value;
			break;
		}
		case e_short: {
			int16_t word = byteStream.readShort();
			size -= 2;
			ASSERT_EQ(word, *(int16_t* ) value);
			delete (int16_t*) value;
			break;
		}
		case e_int: {
			int32_t dword = byteStream.readInt();
			size -= 4;
			ASSERT_EQ(dword, *(int32_t* ) value);
			delete (int32_t*) value;
			break;
		}
		case e_float: {
			float dword = byteStream.readFloat();
			size -= 4;
			ASSERT_DOUBLE_EQ(dword, *(float* ) value);
			delete (float*) value;
			break;
		}
		case e_string: {
			core::String str = byteStream.readString();
			size -= str.size() + 1; //plus the '\0' char
			ASSERT_EQ(str, *(core::String* ) value);
			delete (core::String*) value;
			break;
		}
		default:
			ASSERT_TRUE(false);
			break;
		}
		_typeValueList.erase(_typeValueList.begin());
		ASSERT_EQ(byteStream.getSize(), size);
	} while (index++ < iterations);
	ASSERT_EQ(byteStream.getSize(), size_t(0));
}

}
