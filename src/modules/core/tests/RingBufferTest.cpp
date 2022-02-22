/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/String.h"
#include "core/collection/RingBuffer.h"

namespace core {

namespace _privtest {
struct Type {
	int a;
	int b;

	inline bool operator==(const Type& rhs) const {
		return a == rhs.a && b == rhs.b;
	}
};

struct TypeCtor {
	int a;
	int b;

	TypeCtor(int _a, int _b) : a(_a), b(_b) {
	}
	TypeCtor() : a(0), b(0) {
	}

	inline bool operator==(const Type& rhs) const {
		return a == rhs.a && b == rhs.b;
	}
};
}

TEST(RingBufferTest, testInsert) {
	core::RingBuffer<_privtest::Type, 4> list;
	list.push_back({1, 1});
	EXPECT_EQ(1u, list.size());
	list.emplace_back(2, 2);
	EXPECT_EQ(2u, list.size());
	list.push_back({3, 3});
	EXPECT_EQ(3u, list.size());
}

TEST(RingBufferTest, testInsertCtor) {
	core::RingBuffer<_privtest::TypeCtor, 4> list;
	list.push_back({1, 1});
	EXPECT_EQ(1u, list.size());
	list.emplace_back(2, 2);
	EXPECT_EQ(2u, list.size());
	list.push_back({3, 3});
	EXPECT_EQ(3u, list.size());
}

TEST(RingBufferTest, testPop) {
	core::RingBuffer<_privtest::Type, 4> list;
	list.push_back({1, 1});
	EXPECT_EQ(1u, list.size());
	list.pop();
	EXPECT_EQ(0u, list.size());
}

TEST(RingBufferTest, testWrap) {
	core::RingBuffer<_privtest::Type, 4> list;
	list.push_back({1, 1});
	EXPECT_EQ(1, list[0].a);
	list.push_back({2, 2});
	EXPECT_EQ(2, list[1].a);
	list.push_back({3, 3});
	EXPECT_EQ(3, list[2].a);
	list.push_back({4, 4});
	EXPECT_EQ(4, list[3].a);
	list.push_back({5, 5});
	EXPECT_EQ(2, list[0].a);
	EXPECT_EQ(3, list[1].a);
	EXPECT_EQ(4, list[2].a);
	EXPECT_EQ(5, list[3].a);
	EXPECT_EQ(2, list.begin()->a);
}

TEST(RingBufferTest, testIterate) {
	core::RingBuffer<_privtest::Type> list;
	for (int i = 0; i < 16; ++i) {
		list.push_back({i, i});
	}
	EXPECT_EQ(16u, list.size());
	int cnt = 0;
	for (auto iter = list.begin(); iter != list.end(); ++iter) {
		EXPECT_EQ(iter->a, cnt);
		++cnt;
	}
	EXPECT_EQ(16, cnt);
}

TEST(RingBufferTest, testIterateOverflow) {
	core::RingBuffer<_privtest::Type, 8> list;
	for (int i = 0; i < 16; ++i) {
		list.push_back({i, i});
	}
	EXPECT_EQ(8u, list.size());
	int cnt = 0;
	for (auto iter = list.begin(); iter != list.end(); ++iter) {
		EXPECT_EQ(iter->a, cnt + 8);
		++cnt;
	}
	EXPECT_EQ(8, cnt);
}

TEST(RingBufferTest, testIterateRangeBased) {
	core::RingBuffer<_privtest::Type> list;
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

TEST(RingBufferTest, testStrings) {
	core::RingBuffer<core::String, 3> list;
	list.push_back("foo");
	list.push_back("bar");
	list.push_back("foobar");
	list.push_back("barfoo");
	EXPECT_EQ("bar", list.front());
	EXPECT_EQ("barfoo", list.back());
	for (const core::String& f : list) {
		EXPECT_FALSE(f.empty());
	}
}

}
