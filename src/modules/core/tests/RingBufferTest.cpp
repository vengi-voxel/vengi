/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/RingBuffer.h"

namespace core {

struct Type {
	int a;
	int b;

	inline bool operator==(const Type& rhs) const {
		return a == rhs.a && b == rhs.b;
	}
};

TEST(RingBufferTest, testInsert) {
	core::RingBuffer<Type, 4> list;
	list.push_back({1, 1});
	EXPECT_EQ(1u, list.size());
	list.emplace_back({2, 2});
	EXPECT_EQ(2u, list.size());
	list.push_back({3, 3});
	EXPECT_EQ(3u, list.size());
}

TEST(RingBufferTest, testPop) {
	core::RingBuffer<Type, 4> list;
	list.push_back({1, 1});
	EXPECT_EQ(1u, list.size());
	list.pop();
	EXPECT_EQ(0u, list.size());
}

TEST(RingBufferTest, testWrap) {
	core::RingBuffer<Type, 4> list;
	list.push_back({1, 1});
	EXPECT_EQ(1, list[0].a);
	list.push_back({2, 2});
	EXPECT_EQ(2, list[1].a);
	list.push_back({3, 3});
	EXPECT_EQ(3, list[2].a);
	list.push_back({4, 4});
	EXPECT_EQ(4, list[3].a);
	list.push_back({5, 5});
	EXPECT_EQ(5, list[0].a);
	EXPECT_EQ(2, list[1].a);
	EXPECT_EQ(3, list[2].a);
	EXPECT_EQ(4, list[3].a);
}

TEST(RingBufferTest, testIterate) {
	core::RingBuffer<Type> list;
	for (int i = 0; i < 16; ++i) {
		list.push_back({i, i});
	}
	EXPECT_EQ(16u, list.size());
	int cnt = 0;
	for (auto iter = list.begin(); iter != list.end(); ++iter) {
		++cnt;
	}
	EXPECT_EQ(16, cnt);
}

TEST(RingBufferTest, testIterateRangeBased) {
	core::RingBuffer<Type> list;
	for (int i = 0; i < 16; ++i) {
		list.push_back({i, i});
	}
	EXPECT_EQ(16u, list.size());
	int cnt = 0;
	for (auto iter : list) {
		EXPECT_EQ(cnt, iter.a);
		++cnt;
	}
	EXPECT_EQ(16, cnt);
}

}
