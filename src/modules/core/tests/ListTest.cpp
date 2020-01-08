/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/List.h"

namespace core {

struct Type {
	int a;
	int b;

	inline bool operator==(const Type& rhs) const {
		return a == rhs.a && b == rhs.b;
	}
};

TEST(ListTest, testInsert) {
	core::List<Type> list;
	EXPECT_TRUE(list.insert({1, 1}));
	EXPECT_EQ(1u, list.size());
	EXPECT_TRUE(list.insert({2, 2}));
	EXPECT_EQ(2u, list.size());
	EXPECT_TRUE(list.insert({3, 3}));
	EXPECT_EQ(3u, list.size());
}

TEST(ListTest, testInsertRemove) {
	core::List<Type> list;
	EXPECT_TRUE(list.insert({1, 1}));
	EXPECT_EQ(1u, list.size());
	EXPECT_TRUE(list.remove({1, 1}));
	EXPECT_EQ(0u, list.size());
}

TEST(ListTest, testClear) {
	core::List<Type> list;
	for (int i = 0; i < 16; ++i) {
		EXPECT_TRUE(list.insert({i, i}));
	}
	EXPECT_EQ(16u, list.size());
	EXPECT_FALSE(list.empty());
	list.clear();
	EXPECT_EQ(0u, list.size());
	EXPECT_TRUE(list.empty());
}

TEST(ListTest, testIterator) {
	core::List<Type> list;
	EXPECT_EQ(list.begin(), list.end());
	EXPECT_TRUE(list.insert({1, 1}));
	EXPECT_NE(list.begin(), list.end());
	EXPECT_EQ(++list.begin(), list.end());
}

TEST(ListTest, testIterate) {
	core::List<Type> list;
	for (int i = 0; i < 16; ++i) {
		EXPECT_TRUE(list.insert({i, i}));
	}
	EXPECT_EQ(16u, list.size());
	int cnt = 0;
	for (auto iter = list.begin(); iter != list.end(); ++iter) {
		++cnt;
	}
	EXPECT_EQ(16, cnt);
}

TEST(ListTest, testIterateRangeBased) {
	core::List<Type> list;
	for (int i = 0; i < 16; ++i) {
		EXPECT_TRUE(list.insert({i, i}));
	}
	EXPECT_EQ(16u, list.size());
	int cnt = 0;
	for (auto iter : list) {
		EXPECT_EQ(cnt, iter->value.a);
		++cnt;
	}
	EXPECT_EQ(16, cnt);
}
}
