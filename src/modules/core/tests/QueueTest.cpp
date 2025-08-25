/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/Queue.h"

namespace core {

struct QueueTestType {
	int a;
	int b;

	inline bool operator==(const QueueTestType& rhs) const {
		return a == rhs.a && b == rhs.b;
	}
};

TEST(QueueTest, testPush) {
	core::Queue<QueueTestType> list;
	list.push({1, 1});
	EXPECT_EQ(1u, list.size());
	list.push({2, 2});
	EXPECT_EQ(2u, list.size());
	list.push({3, 3});
	EXPECT_EQ(3u, list.size());
}

TEST(QueueTest, testPop) {
	core::Queue<QueueTestType> list;
	list.push({1, 42});
	EXPECT_EQ(1u, list.size());
	QueueTestType t = list.pop();
	EXPECT_EQ(0u, list.size());
	EXPECT_EQ(1, t.a);
	EXPECT_EQ(42, t.b);
}

TEST(QueueTest, testTryPop) {
	core::Queue<QueueTestType> list;
	QueueTestType val;
	EXPECT_FALSE(list.try_pop(val));
	list.push({1, 42});
	EXPECT_TRUE(list.try_pop(val));
	EXPECT_EQ(0u, list.size());
	EXPECT_EQ(1, val.a);
	EXPECT_EQ(42, val.b);
}

TEST(QueueTest, testResize) {
	core::Queue<QueueTestType, 1> list;
	QueueTestType val;
	EXPECT_FALSE(list.try_pop(val));
	for (int i = 0; i < 10; ++i) {
		list.push({i, i * 10});
		EXPECT_EQ(i + 1u, list.capacity());
		EXPECT_EQ(i + 1u, list.size());
	}
	EXPECT_TRUE(list.try_pop(val));
	EXPECT_EQ(9u, list.size());
	EXPECT_EQ(0, val.a);
	EXPECT_EQ(0, val.b);
	EXPECT_TRUE(list.try_pop(val));
	EXPECT_EQ(8u, list.size());
	EXPECT_EQ(1, val.a);
	EXPECT_EQ(10, val.b);
}

}
