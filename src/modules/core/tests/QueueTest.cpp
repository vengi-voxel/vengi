/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/Queue.h"

namespace core {

struct Type {
	int a;
	int b;

	inline bool operator==(const Type& rhs) const {
		return a == rhs.a && b == rhs.b;
	}
};

TEST(QueueTest, testPush) {
	core::Queue<Type> list;
	list.push({1, 1});
	EXPECT_EQ(1u, list.size());
	list.push({2, 2});
	EXPECT_EQ(2u, list.size());
	list.push({3, 3});
	EXPECT_EQ(3u, list.size());
}

TEST(QueueTest, testPop) {
	core::Queue<Type> list;
	list.push({1, 42});
	EXPECT_EQ(1u, list.size());
	Type t = list.pop();
	EXPECT_EQ(0u, list.size());
	EXPECT_EQ(1, t.a);
	EXPECT_EQ(42, t.b);
}

TEST(QueueTest, testTryPop) {
	core::Queue<Type> list;
	Type val;
	EXPECT_FALSE(list.try_pop(val));
	list.push({1, 42});
	EXPECT_TRUE(list.try_pop(val));
	EXPECT_EQ(0u, list.size());
	EXPECT_EQ(1, val.a);
	EXPECT_EQ(42, val.b);
}

TEST(QueueTest, testRangeBasedForLoop) {
	core::Queue<Type> list;
	list.push({1, 1});
	list.push({2, 2});
	list.push({3, 3});
	EXPECT_EQ(3u, list.size());
	int cnt = 0;
	for (const auto& iter : list) {
		++cnt;
		EXPECT_EQ(cnt, iter.a);
		EXPECT_EQ(cnt, iter.b);
	}
}

}
