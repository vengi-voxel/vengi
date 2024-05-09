/**
 * @file
 */

#include "core/SharedPtr.h"
#include "core/Algorithm.h"
#include "core/Common.h"
#include "core/collection/StringMap.h"
#include <gtest/gtest.h>

namespace core {

struct Foo {
	Foo(int _a, int _b) : a(_a), b(_b) {
	}
	int a;
	int b;
};

struct Bar : Foo {
	Bar(int _a, int _b, int _c) : Foo(_a, _b), c(_c) {
	}
	int c;
};

typedef core::SharedPtr<Foo> FooPtr;
typedef core::SharedPtr<Bar> BarPtr;

class SharedPtrTest : public testing::Test {};

TEST_F(SharedPtrTest, testConvertible) {
	FooPtr p = SharedPtr<Foo>::create(1, 2);
	FooPtr p2 = core::make_shared<Bar>(1, 2, 3);
	core::exchange(p, p2);
	Foo *value = p.get();
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
	p = BarPtr();
}

TEST_F(SharedPtrTest, testAllocate) {
	SharedPtr<Foo> p = SharedPtr<Foo>::create(1, 2);
	Foo *value = p.get();
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
}

TEST_F(SharedPtrTest, testRelease) {
	auto p = SharedPtr<Foo>::create(1, 2);
	auto p2 = p;
	p.release();
	Foo *value = p2.get();
	EXPECT_EQ(nullptr, p.get());
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
}

TEST_F(SharedPtrTest, testMakeShared) {
	auto p = make_shared<Foo>(1, 2);
	Foo *value = p.get();
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
}

TEST_F(SharedPtrTest, testHashMap) {
	StringMap<FooPtr, 3> map;
	map.emplace("1", make_shared<Foo>(1, 2));
	map.emplace("2", make_shared<Foo>(2, 1));
	map.put("3", make_shared<Foo>(3, 4));
	map.put("4", make_shared<Foo>(4, 3));
	map.put("5", make_shared<Foo>(5, 6));
	map.emplace("6", make_shared<Foo>(6, 7));
	map.emplace("6", make_shared<Foo>(6, 7));
	ASSERT_EQ(6u, map.size());
	ASSERT_TRUE(map.remove("1"));
	ASSERT_EQ(5u, map.size());
	map.clear();
}

TEST_F(SharedPtrTest, testMove) {
	FooPtr p1 = make_shared<Foo>(1, 2);
	p1 = core::move(p1);
	EXPECT_EQ((int)*p1.refCnt(), 1);
	FooPtr p2 = p1;
	EXPECT_EQ((int)*p1.refCnt(), 2);
	EXPECT_EQ((int)*p2.refCnt(), 2);
	p1 = p2;
	EXPECT_EQ((int)*p1.refCnt(), 2);
	EXPECT_EQ((int)*p2.refCnt(), 2);
	FooPtr p3(core::move(p1));
	EXPECT_EQ((int)*p3.refCnt(), 2);
	p1 = make_shared<Bar>(1, 2, 3);
	EXPECT_EQ((int)*p1.refCnt(), 1);
	p1.release();
	EXPECT_FALSE(p1);
	p2.release();
	EXPECT_FALSE(p2);
	EXPECT_EQ((int)*p3.refCnt(), 1);
	p3.release();
	EXPECT_FALSE(p3);
}

} // namespace core
