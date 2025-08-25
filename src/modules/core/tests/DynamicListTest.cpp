/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/DynamicList.h"
#include "core/String.h"

namespace core {

struct DynamicListTestType {
	int a;
	int b;
	core::String str;

	DynamicListTestType(int _a, int _b)
		: a(_a), b(_b), str("averylongstringthatexcceedstheinteralbufferandallocatestheapmemory") {
	}

	inline bool operator==(const DynamicListTestType& rhs) const {
		return a == rhs.a && b == rhs.b;
	}
};

TEST(DynamicListTest, testInsert) {
	core::DynamicList<DynamicListTestType> list;
	EXPECT_TRUE(list.insert({1, 1}));
	EXPECT_EQ(1u, list.size());
	EXPECT_TRUE(list.insert({2, 2}));
	EXPECT_EQ(2u, list.size());
	EXPECT_TRUE(list.insert({3, 3}));
	EXPECT_EQ(3u, list.size());
}

TEST(DynamicListTest, testInsertRemove) {
	core::DynamicList<DynamicListTestType> list;
	EXPECT_TRUE(list.insert({1, 1}));
	EXPECT_EQ(1u, list.size());
	EXPECT_TRUE(list.remove({1, 1}));
	EXPECT_EQ(0u, list.size());
	EXPECT_TRUE(list.insert({1, 1}));
	EXPECT_EQ(1u, list.size());
}

TEST(DynamicListTest, testInsertDuplicate) {
	core::DynamicList<DynamicListTestType> list;
	list.insert({1, 1});
	list = {};
}

TEST(DynamicListTest, testClear) {
	core::DynamicList<DynamicListTestType> list;
	for (int i = 0; i < 16; ++i) {
		EXPECT_TRUE(list.insert({i, i}));
	}
	EXPECT_TRUE(list.remove({0, 0}));
	EXPECT_TRUE(list.remove({3, 3}));
	EXPECT_TRUE(list.insert({32, 32}));
	EXPECT_EQ(15u, list.size());
	EXPECT_FALSE(list.empty());
	core::DynamicList<DynamicListTestType> copy(list);
	list.clear();
	EXPECT_EQ(0u, list.size());
	EXPECT_TRUE(list.empty());
	list = copy;
	EXPECT_EQ(15u, list.size());
	EXPECT_FALSE(list.empty());
	list.clear();
	list = {};
	list = core::move(copy);
	EXPECT_EQ(15u, list.size());
	EXPECT_FALSE(list.empty());
}

TEST(DynamicListTest, testIterator) {
	core::DynamicList<DynamicListTestType> list;
	EXPECT_EQ(list.begin(), list.end());
	EXPECT_TRUE(list.insert({1, 1}));
	EXPECT_NE(list.begin(), list.end());
	EXPECT_EQ(++list.begin(), list.end());
}

TEST(DynamicListTest, testIterate) {
	core::DynamicList<DynamicListTestType> list;
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

TEST(DynamicListTest, testIterateRangeBased) {
	core::DynamicList<DynamicListTestType> list;
	for (int i = 0; i < 16; ++i) {
		EXPECT_TRUE(list.insert({i, i}));
	}
	EXPECT_EQ(16u, list.size());
	int cnt = 0;
	for (auto iter : list) {
		EXPECT_EQ(cnt, iter.a);
		++cnt;
	}
	EXPECT_EQ(16, cnt);
}

TEST(DynamicListTest, testErase) {
	core::DynamicList<DynamicListTestType> list;
	for (int i = 0; i < 16; ++i) {
		EXPECT_TRUE(list.insert({i, i}));
	}
	auto iter = list.begin();
	for (int i = 0; i < 4; ++i) {
		EXPECT_EQ(i, iter->value.a);
		++iter;
	}
	EXPECT_EQ(4, iter->value.a);
	auto newIter = list.erase(iter);
	EXPECT_EQ(15u, list.size());
	EXPECT_EQ(5, newIter->value.a);
}

}
