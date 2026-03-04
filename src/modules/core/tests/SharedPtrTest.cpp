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

// WeakPtr tests

TEST_F(SharedPtrTest, testWeakPtrLock) {
	FooPtr shared = make_shared<Foo>(10, 20);
	core::WeakPtr<Foo> weak(shared);
	EXPECT_FALSE(weak.expired());

	FooPtr locked = weak.lock();
	ASSERT_TRUE(locked);
	EXPECT_EQ(10, locked->a);
	EXPECT_EQ(20, locked->b);
	EXPECT_EQ((int)*locked.refCnt(), 2);
}

TEST_F(SharedPtrTest, testWeakPtrExpired) {
	core::WeakPtr<Foo> weak;
	{
		FooPtr shared = make_shared<Foo>(1, 2);
		weak = shared;
		EXPECT_FALSE(weak.expired());
	}
	// shared went out of scope, weak should be expired
	EXPECT_TRUE(weak.expired());
	FooPtr locked = weak.lock();
	EXPECT_FALSE(locked);
}

TEST_F(SharedPtrTest, testWeakPtrCopy) {
	FooPtr shared = make_shared<Foo>(3, 4);
	core::WeakPtr<Foo> weak1(shared);
	core::WeakPtr<Foo> weak2(weak1);
	EXPECT_FALSE(weak1.expired());
	EXPECT_FALSE(weak2.expired());

	FooPtr locked1 = weak1.lock();
	FooPtr locked2 = weak2.lock();
	ASSERT_TRUE(locked1);
	ASSERT_TRUE(locked2);
	EXPECT_EQ(locked1.get(), locked2.get());
}

TEST_F(SharedPtrTest, testWeakPtrMove) {
	FooPtr shared = make_shared<Foo>(5, 6);
	core::WeakPtr<Foo> weak1(shared);
	core::WeakPtr<Foo> weak2(core::move(weak1));
	EXPECT_TRUE(weak1 == nullptr);
	EXPECT_FALSE(weak2.expired());

	FooPtr locked = weak2.lock();
	ASSERT_TRUE(locked);
	EXPECT_EQ(5, locked->a);
	EXPECT_EQ(6, locked->b);
}

TEST_F(SharedPtrTest, testWeakPtrReset) {
	FooPtr shared = make_shared<Foo>(7, 8);
	core::WeakPtr<Foo> weak(shared);
	EXPECT_FALSE(weak.expired());
	weak.reset();
	EXPECT_TRUE(weak == nullptr);
}

TEST_F(SharedPtrTest, testWeakPtrAssignNullptr) {
	FooPtr shared = make_shared<Foo>(9, 10);
	core::WeakPtr<Foo> weak(shared);
	EXPECT_FALSE(weak.expired());
	weak = nullptr;
	EXPECT_TRUE(weak == nullptr);
}

TEST_F(SharedPtrTest, testWeakPtrConvertible) {
	BarPtr barShared = make_shared<Bar>(1, 2, 3);
	core::WeakPtr<Foo> weakFoo(barShared);
	EXPECT_FALSE(weakFoo.expired());

	FooPtr locked = weakFoo.lock();
	ASSERT_TRUE(locked);
	EXPECT_EQ(1, locked->a);
	EXPECT_EQ(2, locked->b);
}

TEST_F(SharedPtrTest, testWeakPtrMultipleObservers) {
	core::WeakPtr<Foo> weak1;
	core::WeakPtr<Foo> weak2;
	{
		FooPtr shared = make_shared<Foo>(11, 12);
		weak1 = shared;
		weak2 = shared;
		EXPECT_FALSE(weak1.expired());
		EXPECT_FALSE(weak2.expired());
	}
	// Both should be expired now
	EXPECT_TRUE(weak1.expired());
	EXPECT_TRUE(weak2.expired());
	EXPECT_FALSE(weak1.lock());
	EXPECT_FALSE(weak2.lock());
}

TEST_F(SharedPtrTest, testWeakPtrDefaultConstructor) {
	core::WeakPtr<Foo> weak;
	EXPECT_TRUE(weak.expired());
	EXPECT_TRUE(weak == nullptr);
	EXPECT_FALSE(weak.lock());
}

TEST_F(SharedPtrTest, testWeakPtrAssignFromShared) {
	FooPtr shared1 = make_shared<Foo>(1, 2);
	FooPtr shared2 = make_shared<Foo>(3, 4);
	core::WeakPtr<Foo> weak(shared1);

	FooPtr locked = weak.lock();
	ASSERT_TRUE(locked);
	EXPECT_EQ(1, locked->a);
	locked.release();

	weak = shared2;
	locked = weak.lock();
	ASSERT_TRUE(locked);
	EXPECT_EQ(3, locked->a);
}

} // namespace core
