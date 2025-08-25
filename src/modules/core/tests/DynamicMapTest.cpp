/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/DynamicMap.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicStringMap.h"

namespace core {

TEST(DynamicMapTest, testPutGet) {
	core::DynamicMap<int64_t, int64_t, 11, std::hash<int64_t>> map;
	map.put(1, 1);
	map.put(1, 2);
	map.put(2, 1);
	map.put(3, 1337);
	map.put(4, 42);
	map.put(5, 111);
	map.put(6, 1111);
	int64_t value;
	EXPECT_TRUE(map.get(1, value));
	EXPECT_EQ(2, value);
	EXPECT_TRUE(map.get(2, value));
	EXPECT_EQ(1, value);
	EXPECT_TRUE(map.get(3, value));
	EXPECT_EQ(1337, value);
	EXPECT_TRUE(map.get(4, value));
	EXPECT_EQ(42, value);
	EXPECT_TRUE(map.get(5, value));
	EXPECT_EQ(111, value);
	EXPECT_TRUE(map.get(6, value));
	EXPECT_EQ(1111, value);
}

TEST(DynamicMapTest, testCollision) {
	core::DynamicMap<int64_t, int64_t, 11, std::hash<int64_t>> map;
	for (int64_t i = 0; i < 128; ++i) {
		map.put(i, i);
	}
	int64_t value = 0;
	for (int64_t i = 0; i < 128; ++i) {
		EXPECT_TRUE(map.get(i, value));
		EXPECT_EQ(i, value);
	}
}

TEST(DynamicMapTest, testClear) {
	core::DynamicMap<int64_t, int64_t, 11, std::hash<int64_t>> map;
	for (int64_t i = 0; i < 16; ++i) {
		map.put(i, i);
	}
	EXPECT_EQ(16u, map.size());
	EXPECT_FALSE(map.empty());
	map.clear();
	EXPECT_EQ(0u, map.size());
	EXPECT_TRUE(map.empty());
}

TEST(DynamicMapTest, testFind) {
	core::DynamicMap<int64_t, int64_t, 11, std::hash<int64_t>> map;
	for (int64_t i = 0; i < 1024; i += 2) {
		map.put(i, i);
	}
	auto iter = map.find(0);
	EXPECT_NE(map.end(), iter);
	EXPECT_EQ(0u, iter->value);

	iter = map.find(1);
	EXPECT_EQ(map.end(), iter);
}

TEST(DynamicMapTest, testIterator) {
	core::DynamicMap<int64_t, int64_t, 11, std::hash<int64_t>> map;
	EXPECT_EQ(map.begin(), map.end());
	EXPECT_EQ(map.end(), map.find(42));
	map.put(1, 1);
	EXPECT_NE(map.begin(), map.end());
	EXPECT_EQ(++map.begin(), map.end());
}

TEST(DynamicMapTest, testIterate) {
	// leave empty buckets
	core::DynamicMap<int64_t, int64_t, 11, std::hash<int64_t>> map;
	for (int64_t i = 0; i < 32; i += 2) {
		map.put(i, i);
	}
	EXPECT_EQ(16u, map.size());
	int cnt = 0;
	for (auto iter = map.begin(); iter != map.end(); ++iter) {
		++cnt;
	}
	EXPECT_EQ(16, cnt);

	for (int64_t i = 0; i < 1024; ++i) {
		map.put(i, i);
	}
	EXPECT_EQ(1024u, map.size());
	cnt = 0;
	for (auto iter = map.begin(); iter != map.end(); ++iter) {
		++cnt;
	}
	EXPECT_EQ(1024, cnt);
}

TEST(DynamicMapTest, testIterateRangeBased) {
	core::DynamicMap<int64_t, int64_t, 11, std::hash<int64_t>> map;
	for (int64_t i = 0; i < 32; i += 2) {
		map.put(i, i);
	}
	EXPECT_EQ(16u, map.size());
	int cnt = 0;
	for (auto iter : map) {
		EXPECT_EQ(iter->key, iter->value);
		++cnt;
	}
	EXPECT_EQ(16, cnt);
}

TEST(DynamicMapTest, testStringSharedPtr) {
	core::DynamicStringMap<core::SharedPtr<core::String>, 4> map;
	auto foobar = core::make_shared<core::String>("foobar");
	map.put("foobar", foobar);
	map.put("barfoo", core::make_shared<core::String>("barfoo"));
	map.put("foobar", core::make_shared<core::String>("barfoo"));
	for (auto iter = map.begin(); iter != map.end(); ++iter) {
	}
	map.clear();
	foobar = {};
}

TEST(DynamicMapTest, testCopy) {
	core::DynamicStringMap<core::SharedPtr<core::String>> map;
	map.put("foobar", core::make_shared<core::String>("barfoo"));
	auto map2 = map;
	map2.clear();
}

TEST(DynamicMapTest, testCopyBlocks) {
	core::DynamicStringMap<core::SharedPtr<core::String>> map;
	for (int i = 0; i < 1024; ++i) {
		map.put(core::string::toString(i), core::make_shared<core::String>("barfoo"));
	}
	auto map2 = map;
	map2.clear();
}

TEST(DynamicMapTest, testCopyBlocksMove) {
	core::DynamicStringMap<core::SharedPtr<core::String>> map;
	for (int i = 0; i < 1024; ++i) {
		map.put(core::string::toString(i), core::make_shared<core::String>("barfoo"));
	}
	core::DynamicStringMap<core::SharedPtr<core::String>> map2;
	map2 = core::move(map);
}

TEST(DynamicMapTest, testMove) {
	core::DynamicStringMap<core::SharedPtr<core::String>> map2;
	{
		core::DynamicStringMap<core::SharedPtr<core::String>> map;
		map.put("foobar", core::make_shared<core::String>("barfoo"));
		EXPECT_NE(map.end(), map.find("foobar"));
		map2 = core::move(map);
		EXPECT_EQ(map.end(), map.find("foobar"));
		map.clear();
	}
	EXPECT_NE(map2.end(), map2.find("foobar"));
	EXPECT_EQ(map2.end(), map2.find("foobar2"));
	map2.clear();
}

TEST(DynamicMapTest, testErase) {
	core::DynamicStringMap<core::SharedPtr<core::String>> map;
	map.put("foobar", core::make_shared<core::String>("barfoo"));
	EXPECT_EQ(1u, map.size());
	auto iter = map.find("foobar");
	EXPECT_NE(iter, map.end());
	map.erase(iter);
	EXPECT_EQ(0u, map.size());
}

TEST(DynamicMapTest, testAssign) {
	core::DynamicStringMap<core::SharedPtr<core::String>> map;
	map.put("foobar", core::make_shared<core::String>("barfoo"));
	core::DynamicStringMap<core::SharedPtr<core::String>> map2;
	map2 = map;
	EXPECT_EQ(1u, map.size());
	EXPECT_EQ(1u, map2.size());
	map2.clear();
	EXPECT_EQ(1u, map.size());
	EXPECT_EQ(0u, map2.size());
}

}
