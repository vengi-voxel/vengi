/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/MRUBuffer.h"

namespace core {

TEST(MRUBufferTest, testPushAndOrder) {
	core::MRUBuffer<int, 4> buf;
	buf.push(1);
	buf.push(2);
	buf.push(3);
	EXPECT_EQ(3u, buf.size());
	EXPECT_EQ(3, buf[0]);
	EXPECT_EQ(2, buf[1]);
	EXPECT_EQ(1, buf[2]);
}

TEST(MRUBufferTest, testDuplicateMovesToFront) {
	core::MRUBuffer<int, 4> buf;
	buf.push(1);
	buf.push(2);
	buf.push(3);
	// push duplicate - should move to front
	buf.push(1);
	EXPECT_EQ(3u, buf.size());
	EXPECT_EQ(1, buf[0]);
	EXPECT_EQ(3, buf[1]);
	EXPECT_EQ(2, buf[2]);
}

TEST(MRUBufferTest, testEviction) {
	core::MRUBuffer<int, 3> buf;
	buf.push(1);
	buf.push(2);
	buf.push(3);
	buf.push(4);
	EXPECT_EQ(3u, buf.size());
	EXPECT_EQ(4, buf[0]);
	EXPECT_EQ(3, buf[1]);
	EXPECT_EQ(2, buf[2]);
}

TEST(MRUBufferTest, testDuplicateDoesNotEvict) {
	core::MRUBuffer<int, 3> buf;
	buf.push(1);
	buf.push(2);
	buf.push(3);
	buf.push(2);
	EXPECT_EQ(3u, buf.size());
	EXPECT_EQ(2, buf[0]);
	EXPECT_EQ(3, buf[1]);
	EXPECT_EQ(1, buf[2]);
}

TEST(MRUBufferTest, testEmpty) {
	core::MRUBuffer<int, 4> buf;
	EXPECT_TRUE(buf.empty());
	EXPECT_EQ(0u, buf.size());
	buf.push(1);
	EXPECT_FALSE(buf.empty());
}

TEST(MRUBufferTest, testClear) {
	core::MRUBuffer<int, 4> buf;
	buf.push(1);
	buf.push(2);
	buf.clear();
	EXPECT_TRUE(buf.empty());
	EXPECT_EQ(0u, buf.size());
}

TEST(MRUBufferTest, testIterate) {
	core::MRUBuffer<int, 4> buf;
	buf.push(1);
	buf.push(2);
	buf.push(3);
	int expected[] = {3, 2, 1};
	int idx = 0;
	for (int val : buf) {
		EXPECT_EQ(expected[idx], val);
		++idx;
	}
	EXPECT_EQ(3, idx);
}

TEST(MRUBufferTest, testCapacity) {
	core::MRUBuffer<int, 5> buf;
	EXPECT_EQ(5u, buf.capacity());
}

}
