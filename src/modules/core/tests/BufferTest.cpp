/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/Buffer.h"
#include "core/String.h"

namespace core {

template<class TYPE, size_t SIZE>
::std::ostream& operator<<(::std::ostream& ostream, const Buffer<TYPE, SIZE>& v) {
	int idx = 0;
	for (auto i = v.begin(); i != v.end();) {
		ostream << "'";
		ostream << *i;
		ostream << "' (" << idx << ")";
		if (++i != v.end()) {
			ostream << ", ";
		}
		++idx;
	}
	return ostream;
}

TEST(BufferTest, testPushBack) {
	Buffer<uint8_t> array;
	array.push_back(0);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(BufferTest, testPushBackInt) {
	Buffer<uint32_t> array;
	array.push_back(1337);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(1337u, array[0]) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(BufferTest, testClear) {
	Buffer<uint8_t> array;
	array.push_back(0);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	array.clear();
	EXPECT_EQ(0u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(BufferTest, testRelease) {
	Buffer<uint8_t> array;
	array.push_back(0);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	array.release();
	EXPECT_EQ(0u, array.size()) << array;
	EXPECT_EQ(0u, array.capacity()) << array;
}

TEST(BufferTest, testIterate) {
	Buffer<uint8_t> array;
	array.push_back(1);
	array.push_back(2);
	array.push_back(3);
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	uint8_t i = 1;
	for (const uint8_t& d : array) {
		EXPECT_EQ(i++, d) << array;
	}
}

TEST(BufferTest, testIterateBig) {
	Buffer<int> array;
	array.reserve(10000);
	for (int i = 0; i < (int)array.capacity(); ++i) {
		array.push_back(i);
	}
	int i = 0;
	for (const int& d : array) {
		EXPECT_EQ(i++, d);
	}
}

TEST(BufferTest, testCopy) {
	Buffer<uint8_t> array;
	array.push_back(1);
	array.push_back(2);
	array.push_back(3);
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	Buffer<uint8_t> copy(array);
	EXPECT_EQ(3u, copy.size()) << array;
	EXPECT_EQ(32u, copy.capacity()) << array;
	EXPECT_EQ(1, copy[0]);
	EXPECT_EQ(2, copy[1]);
	EXPECT_EQ(3, copy[2]);
}

TEST(BufferTest, testInsert) {
	Buffer<uint8_t> array;
	array.insert(array.begin(), 3);
	array.insert(array.begin(), 2);
	array.insert(array.begin(), 1);
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	Buffer<uint8_t> copy(array);
	EXPECT_EQ(3u, copy.size()) << array;
	EXPECT_EQ(32u, copy.capacity()) << array;
	EXPECT_EQ(1, copy[0]);
	EXPECT_EQ(2, copy[1]);
	EXPECT_EQ(3, copy[2]);
}

TEST(BufferTest, testTriggerResize) {
	Buffer<uint8_t, 2> array;
	array.push_back(1);
	array.push_back(2);
	EXPECT_EQ(2u, array.size()) << array;
	EXPECT_EQ(2u, array.capacity()) << array;
	array.push_back(3);
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(4u, array.capacity()) << array;
}

TEST(BufferTest, testErase) {
	Buffer<uint8_t, 32> array;
	for (uint8_t i = 0; i < 128; ++i) {
		array.push_back(i);
		EXPECT_EQ(i, array[i]);
	}
	EXPECT_EQ(0, array[0]) << array;
	EXPECT_EQ(127, array[127]) << array;
	EXPECT_EQ(128u, array.size()) << array;
	EXPECT_EQ(128u, array.capacity()) << array;
	array.erase(0, 10);
	EXPECT_EQ(118u, array.size()) << array;
	EXPECT_EQ(10, array[0]) << array;
	array.erase(1, 10);
	EXPECT_EQ(108u, array.size()) << array;
	EXPECT_EQ(10, array[0]) << array;
	array.erase(100, 100);
	EXPECT_EQ(100u, array.size()) << array;
	EXPECT_EQ(10, array[0]) << array;
	EXPECT_EQ(119, array[99]) << array;
	array.erase(0, 99);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(119, array[0]) << array;
	array.erase(0, 1);
	EXPECT_EQ(0u, array.size()) << array;
}

}
