/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/DynamicMultiMap.h"
#include "core/SharedPtr.h"
#include "core/String.h"

namespace core {

TEST(DynamicMultiMapTest, testInsertMultipleValuesForSameKey) {
	core::DynamicMultiMap<int, int> map;
	map.insert(1, 10);
	map.insert(1, 20);
	map.insert(2, 30);
	map.insert(1, 40);
	EXPECT_EQ(4u, map.size());
	EXPECT_EQ(3u, map.count(1));
	EXPECT_EQ(1u, map.count(2));
	EXPECT_EQ(0u, map.count(3));
	EXPECT_TRUE(map.hasKey(1));
	EXPECT_FALSE(map.hasKey(3));
}

TEST(DynamicMultiMapTest, testEqualRange) {
	core::DynamicMultiMap<int, int> map;
	map.insert(1, 10);
	map.insert(2, 20);
	map.insert(1, 11);
	map.insert(3, 30);
	map.insert(1, 12);

	auto range = map.equal_range(1);
	int sum = 0;
	int n = 0;
	for (auto i = range.first; i != range.second; ++i) {
		EXPECT_EQ(1, i->first);
		sum += i->second;
		++n;
	}
	EXPECT_EQ(3, n);
	EXPECT_EQ(33, sum);

	range = map.equal_range(99);
	EXPECT_EQ(range.first, range.second);
	EXPECT_EQ(map.end(), range.first);
}

TEST(DynamicMultiMapTest, testEqualRangePreservesInsertOrder) {
	core::DynamicMultiMap<int, int> map;
	map.insert(7, 1);
	map.insert(7, 2);
	map.insert(7, 3);
	auto range = map.equal_range(7);
	auto i = range.first;
	ASSERT_NE(i, range.second);
	EXPECT_EQ(1, i->second);
	++i;
	ASSERT_NE(i, range.second);
	EXPECT_EQ(2, i->second);
	++i;
	ASSERT_NE(i, range.second);
	EXPECT_EQ(3, i->second);
	++i;
	EXPECT_EQ(i, range.second);
}

TEST(DynamicMultiMapTest, testEqualRangeWithBucketCollisions) {
	core::DynamicMultiMap<int, int, 11> map;
	map.insert(1, 100);
	map.insert(12, 200);
	map.insert(1, 101);
	map.insert(23, 300);
	map.insert(1, 102);

	EXPECT_EQ(5u, map.size());
	EXPECT_EQ(3u, map.count(1));
	EXPECT_EQ(1u, map.count(12));
	EXPECT_EQ(1u, map.count(23));

	auto range = map.equal_range(1);
	int n = 0;
	int sum = 0;
	for (auto i = range.first; i != range.second; ++i) {
		EXPECT_EQ(1, i->key);
		sum += i->value;
		++n;
	}
	EXPECT_EQ(3, n);
	EXPECT_EQ(303, sum);

	range = map.equal_range(12);
	ASSERT_NE(range.first, range.second);
	EXPECT_EQ(200, range.first->second);
	auto next = range.first;
	++next;
	EXPECT_EQ(next, range.second);
}

TEST(DynamicMultiMapTest, testEraseSingleValueKeepsOtherMatches) {
	core::DynamicMultiMap<int, int> map;
	map.insert(1, 10);
	map.insert(1, 20);
	map.insert(1, 30);
	auto range = map.equal_range(1);
	auto it = range.first;
	++it;
	EXPECT_EQ(20, it->second);
	EXPECT_TRUE(map.erase(it));
	EXPECT_EQ(2u, map.size());
	EXPECT_EQ(2u, map.count(1));

	int sum = 0;
	range = map.equal_range(1);
	for (auto i = range.first; i != range.second; ++i) {
		sum += i->second;
	}
	EXPECT_EQ(40, sum);
}

TEST(DynamicMultiMapTest, testRemoveAllForKey) {
	core::DynamicMultiMap<int, int> map;
	map.insert(1, 10);
	map.insert(2, 20);
	map.insert(1, 11);
	EXPECT_EQ(2u, map.remove(1));
	EXPECT_EQ(1u, map.size());
	EXPECT_FALSE(map.hasKey(1));
	EXPECT_TRUE(map.hasKey(2));
	EXPECT_EQ(0u, map.remove(1));
}

TEST(DynamicMultiMapTest, testIterate) {
	core::DynamicMultiMap<int, int, 11> map;
	for (int i = 0; i < 32; i += 2) {
		map.insert(i, i);
		map.insert(i, i + 1000);
	}
	EXPECT_EQ(32u, map.size());
	int cnt = 0;
	for (auto iter = map.begin(); iter != map.end(); ++iter) {
		++cnt;
	}
	EXPECT_EQ(32, cnt);

	cnt = 0;
	for (const auto &entry : map) {
		EXPECT_TRUE(entry.first == entry.second || entry.first + 1000 == entry.second);
		++cnt;
	}
	EXPECT_EQ(32, cnt);
}

TEST(DynamicMultiMapTest, testMutateThroughIterator) {
	core::DynamicMultiMap<int, core::String> map;
	map.insert(1, core::String("foo"));
	map.insert(1, core::String("bar"));
	auto range = map.equal_range(1);
	range.first->second = "baz";
	EXPECT_EQ("baz", range.first->second);
}

TEST(DynamicMultiMapTest, testClear) {
	core::DynamicMultiMap<int, int> map;
	map.insert(1, 1);
	map.insert(1, 2);
	map.insert(2, 3);
	EXPECT_FALSE(map.empty());
	map.clear();
	EXPECT_EQ(0u, map.size());
	EXPECT_TRUE(map.empty());
	EXPECT_EQ(map.begin(), map.end());
	map.insert(3, 4);
	EXPECT_EQ(1u, map.size());
}

TEST(DynamicMultiMapTest, testCopy) {
	core::DynamicMultiMap<int, core::String> map;
	map.insert(1, core::String("a"));
	map.insert(1, core::String("b"));
	map.insert(2, core::String("c"));
	core::DynamicMultiMap<int, core::String> copy(map);
	EXPECT_EQ(3u, copy.size());
	EXPECT_EQ(2u, copy.count(1));
	EXPECT_EQ(3u, map.size());
	copy.clear();
	EXPECT_EQ(3u, map.size());
}

TEST(DynamicMultiMapTest, testAssign) {
	core::DynamicMultiMap<int, int> map;
	map.insert(1, 10);
	map.insert(1, 20);
	core::DynamicMultiMap<int, int> other;
	other = map;
	EXPECT_EQ(2u, other.size());
	EXPECT_EQ(2u, map.size());
	other.clear();
	EXPECT_EQ(2u, map.size());
	map = other;
	EXPECT_TRUE(map.empty());
}

TEST(DynamicMultiMapTest, testMove) {
	core::DynamicMultiMap<int, core::SharedPtr<core::String>> map2;
	{
		core::DynamicMultiMap<int, core::SharedPtr<core::String>> map;
		map.insert(1, core::make_shared<core::String>("foobar"));
		map.insert(1, core::make_shared<core::String>("barfoo"));
		EXPECT_NE(map.end(), map.find(1));
		map2 = core::move(map);
		EXPECT_EQ(map.end(), map.find(1));
		map.clear();
	}
	EXPECT_EQ(2u, map2.size());
	EXPECT_NE(map2.end(), map2.find(1));
	map2.clear();
}

TEST(DynamicMultiMapTest, testCopyBlocks) {
	core::DynamicMultiMap<int, core::SharedPtr<core::String>> map;
	for (int i = 0; i < 1024; ++i) {
		map.insert(i % 32, core::make_shared<core::String>("barfoo"));
	}
	EXPECT_EQ(1024u, map.size());
	auto map2 = map;
	EXPECT_EQ(1024u, map2.size());
	EXPECT_EQ(32u, map2.count(0));
	map2.clear();
	EXPECT_EQ(1024u, map.size());
}

TEST(DynamicMultiMapTest, testFind) {
	core::DynamicMultiMap<int, int> map;
	map.insert(0, 1);
	map.insert(2, 3);
	auto iter = map.find(0);
	EXPECT_NE(map.end(), iter);
	EXPECT_EQ(1, iter->value);
	iter = map.find(1);
	EXPECT_EQ(map.end(), iter);
}

}
