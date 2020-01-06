/**
 * @file
 */

#include "core/PoolAllocator.h"
#include "core/ArrayLength.h"
#include "AbstractTest.h"

namespace core {

class PoolAllocatorTest: public AbstractTest {
protected:
	static int calledCtor;
	static int calledDtor;
	static int calledParamCtor;

	struct Foo {
		int64_t a = 0;
		Foo() {
			++calledCtor;
		}
		Foo(int64_t param) : a(param) {
			++calledParamCtor;
		}
		~Foo() {
			++calledDtor;
		}
	};
	using IntAllocator = PoolAllocator<int64_t>;
	const IntAllocator::SizeType size = 1024;

	using FooAllocator = PoolAllocator<Foo>;

	void SetUp() override {
		calledCtor = 0;
		calledDtor = 0;
		calledParamCtor = 0;
	}
};

int PoolAllocatorTest::calledCtor = 0;
int PoolAllocatorTest::calledDtor = 0;
int PoolAllocatorTest::calledParamCtor = 0;

TEST_F(PoolAllocatorTest, testInit) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size)) << "Failed to init the pool allocator";
	EXPECT_EQ(size, a.max());
	EXPECT_EQ(0, a.allocated());
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testMaxSize) {
	IntAllocator a;
	IntAllocator::PointerType foo[4] {nullptr, nullptr, nullptr, nullptr};
	const IntAllocator::Type n = lengthof(foo);
	ASSERT_TRUE(a.init(n)) << "Failed to init the pool allocator";
	for (IntAllocator::Type i = 0; i < n; ++i) {
		EXPECT_EQ(i, a.allocated()) << "Counter for allocated items did not increase properly";
		foo[i] = a.alloc();
		EXPECT_NE(nullptr, foo[i]) << "Failed to allocate " << i << "th item";
	}
	EXPECT_EQ(n, a.allocated()) << "Could not allocate expected items";
	EXPECT_EQ(nullptr, a.alloc()) << "There are more than the allowed slots in the pool";
	for (IntAllocator::Type i = 0; i < n; ++i) {
		EXPECT_EQ(n - i, a.allocated()) << "Counter for allocated items did not decrease properly";
		EXPECT_TRUE(a.free(foo[i])) << "Failed to free " << i << "th item";
	}
	EXPECT_EQ(0, a.allocated()) << "Could not free all allocated items";
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testAllocFree) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size)) << "Failed to init the pool allocator";
	IntAllocator::PointerType t = a.alloc();
	ASSERT_NE(nullptr, t);
	EXPECT_TRUE(a.free(t));
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testFreeInvalid) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size)) << "Failed to init the pool allocator";
	EXPECT_FALSE(a.free((IntAllocator::PointerType)-1));
	EXPECT_FALSE(a.free((IntAllocator::PointerType)0));
	EXPECT_FALSE(a.free(nullptr));
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testFooClassTypeCtorDtor) {
	FooAllocator a;
	ASSERT_TRUE(a.init(size)) << "Failed to init the pool allocator";
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

	FooAllocator::PointerType t2 = a.alloc(42);
	ASSERT_NE(nullptr, t2);
	EXPECT_TRUE(a.free(t2));
	EXPECT_EQ(1, calledCtor);
	EXPECT_EQ(2, calledParamCtor);
	EXPECT_EQ(3, calledDtor);

	FooAllocator::PointerType t3 = a.alloc(42);
	ASSERT_NE(nullptr, t3);
	EXPECT_TRUE(a.free(t3));
	EXPECT_EQ(1, calledCtor);
	EXPECT_EQ(3, calledParamCtor);
	EXPECT_EQ(4, calledDtor);

	a.shutdown();
}

}
