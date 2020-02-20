/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/SharedPtr.h"

namespace core {

struct SharedPtrFoo {
	SharedPtrFoo(int _a, int _b) : a(_a), b(_b) {}
	int a;
	int b;
};

class PtrTest: public testing::Test {
};

TEST_F(PtrTest, testAllocate) {
	SharedPtr<SharedPtrFoo> p = SharedPtr<SharedPtrFoo>::create(1, 2);
	SharedPtrFoo* value = p.get();
	ASSERT_NE(nullptr, value);
	EXPECT_EQ(1, value->a);
	EXPECT_EQ(2, value->b);
}

TEST_F(PtrTest, testAssign) {
	auto p = SharedPtr<SharedPtrFoo>::create(1, 2);
	auto p2 = p;
	p.release();
}

}
