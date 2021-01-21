/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/ConcurrentDynamicArray.h"
#include <thread>

namespace collection {

class ConcurrentDynamicArrayTest : public testing::Test {
};

TEST_F(ConcurrentDynamicArrayTest, testPushPop) {
	const int n = 1000;
	core::ConcurrentDynamicArray<int> array(n);
	for (int i = 0; i < n; ++i) {
		array.push(i);
	}
	ASSERT_EQ((int)array.size(), n);
	for (int i = n - 1; i >= 0; --i) {
		int v;
		ASSERT_TRUE(array.pop(v));
		ASSERT_EQ(i, v);
	}
}

}
