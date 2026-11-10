/**
 * @file
 */

#include "core/concurrent/Atomic.h"
#include <gtest/gtest.h>

namespace core {

TEST(AtomicPtrTest, CompareExchangeReportsSuccessAndKeepsFailedValue) {
	int first = 1, second = 2;
	AtomicPtr<int> pointer;
	EXPECT_EQ(nullptr, pointer.load());
	EXPECT_TRUE(pointer.compare_exchange(nullptr, &first));
	EXPECT_EQ(&first, pointer.load());
	EXPECT_FALSE(pointer.compare_exchange(nullptr, &second));
	EXPECT_EQ(&first, pointer.load());
	EXPECT_TRUE(pointer.compare_exchange(&first, &second));
	EXPECT_EQ(&second, pointer.load());
	EXPECT_TRUE(pointer.compare_exchange(&second, nullptr));
	EXPECT_EQ(nullptr, pointer.load());
}

TEST(AtomicPtrTest, CopiesAndConstLoadsObservePointer) {
	int first = 1, second = 2;
	const AtomicPtr<int> source(&first);
	EXPECT_EQ(&first, source.load());
	AtomicPtr<int> copy(source);
	EXPECT_EQ(&first, copy.load());
	copy = &second;
	EXPECT_EQ(&first, source.load());
	EXPECT_EQ(&second, copy.load());
	copy = source;
	EXPECT_EQ(&first, copy.load());
	AtomicPtr<int> &same = copy;
	copy = same;
	EXPECT_EQ(&first, copy.load());
	EXPECT_EQ(&first, copy.exchange(nullptr));
	EXPECT_EQ(nullptr, copy.load());
}

} // namespace core
