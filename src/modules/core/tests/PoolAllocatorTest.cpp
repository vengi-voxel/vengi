/**
 * @file
 */

#include "core/PoolAllocator.h"
#include "AbstractTest.h"

namespace core {

class PoolAllocatorTest: public AbstractTest {
protected:
	static int calledCtor;
	static int calledDtor;
	static int calledParamCtor;

	struct Foo {
		int a = 0;
		Foo() {
			++calledCtor;
		}
		Foo(int param) : a(param) {
			++calledParamCtor;
		}
		~Foo() {
			++calledDtor;
		}
	};
	using IntAllocator = PoolAllocator<int>;
	const IntAllocator::SizeType size = 1024;

	using FooAllocator = PoolAllocator<Foo>;

	void SetUp() override {
		calledCtor = 0;
		calledDtor = 0;
	}
};

int PoolAllocatorTest::calledCtor = 0;
int PoolAllocatorTest::calledDtor = 0;
int PoolAllocatorTest::calledParamCtor = 0;

TEST_F(PoolAllocatorTest, testInit) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size));
	EXPECT_EQ(size, a.max());
	EXPECT_EQ(0, a.allocated());
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testAllocFree) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size));
	IntAllocator::PointerType t = a.alloc();
	ASSERT_NE(nullptr, t);
	EXPECT_TRUE(a.free(t));
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testFreeInvalid) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size));
	EXPECT_FALSE(a.free((IntAllocator::PointerType)-1));
	EXPECT_FALSE(a.free((IntAllocator::PointerType)0));
	EXPECT_FALSE(a.free(nullptr));
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testFooClassTypeCtorDtor) {
	FooAllocator a;
	ASSERT_TRUE(a.init(size));
	EXPECT_EQ(0, calledCtor);
	EXPECT_EQ(0, calledParamCtor);
	EXPECT_EQ(0, calledDtor);

	FooAllocator::PointerType t = a.alloc();
	ASSERT_NE(nullptr, t);
	EXPECT_TRUE(a.free(t));
	EXPECT_EQ(1, calledCtor);
	EXPECT_EQ(0, calledParamCtor);
	EXPECT_EQ(1, calledDtor);

	t = a.alloc(42);
	ASSERT_NE(nullptr, t);
	EXPECT_TRUE(a.free(t));
	EXPECT_EQ(1, calledCtor);
	EXPECT_EQ(1, calledParamCtor);
	EXPECT_EQ(2, calledDtor);

	a.shutdown();
}

}
