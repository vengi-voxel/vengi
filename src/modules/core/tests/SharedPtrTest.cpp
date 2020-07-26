/**
 * @file
 */

#include "core/Common.h"
#include "core/tests/AbstractTest.h"
#include "core/SharedPtr.h"
#include "core/Algorithm.h"

namespace core {

struct SharedPtrFoo {
	SharedPtrFoo(int _a, int _b) : a(_a), b(_b) {}
	int a;
	int b;
};

struct SharedPtrBar : SharedPtrFoo {
	SharedPtrBar(int _a, int _b, int _c) : SharedPtrFoo(_a, _b), c(_c) {}
	int c;
};

typedef core::SharedPtr<SharedPtrFoo> FooPtr;
typedef core::SharedPtr<SharedPtrBar> BarPtr;

class PtrTest: public testing::Test {
};

TEST_F(PtrTest, testConvertible) {
	FooPtr p = SharedPtr<SharedPtrFoo>::create(1, 2);
	FooPtr p2 = core::make_shared<SharedPtrBar>(1, 2, 3);
	core::exchange(p, p2);
	SharedPtrFoo* value = p.get();
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
	p = BarPtr();
}

TEST_F(PtrTest, testAllocate) {
	SharedPtr<SharedPtrFoo> p = SharedPtr<SharedPtrFoo>::create(1, 2);
	SharedPtrFoo* value = p.get();
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
}

TEST_F(PtrTest, testRelease) {
	auto p = SharedPtr<SharedPtrFoo>::create(1, 2);
	auto p2 = p;
	p.release();
	SharedPtrFoo* value = p2.get();
	EXPECT_EQ(nullptr, p.get());
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
}

TEST_F(PtrTest, testMakeShared) {
	auto p = make_shared<SharedPtrFoo>(1, 2);
	SharedPtrFoo* value = p.get();
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
}

}
