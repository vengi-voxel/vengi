/**
 * @file
 */

#include "core/PoolAllocator.h"
#include "AbstractTest.h"

namespace core {

class PoolAllocatorTest: public AbstractTest {
protected:
	using IntAllocator = PoolAllocator<int>;
	const IntAllocator::SizeType size = 1024;
};

TEST_F(PoolAllocatorTest, testInit) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size));
	ASSERT_EQ(size, a.max());
	ASSERT_EQ(0, a.allocated());
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testAlloc) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size));
	ASSERT_NE(nullptr, a.alloc());
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testFree) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size));
	IntAllocator::PointerType t = a.alloc();
	ASSERT_NE(nullptr, t);
	ASSERT_TRUE(a.free(t));
	a.shutdown();
}

TEST_F(PoolAllocatorTest, testFreeInvalid) {
	IntAllocator a;
	ASSERT_TRUE(a.init(size));
	ASSERT_FALSE(a.free((IntAllocator::PointerType)-1));
	ASSERT_FALSE(a.free((IntAllocator::PointerType)0));
	ASSERT_FALSE(a.free(nullptr));
	a.shutdown();
}

}
