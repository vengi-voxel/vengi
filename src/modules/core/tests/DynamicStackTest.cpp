/**
 * @file
 */

#include "core/collection/DynamicStack.h"
#include <gtest/gtest.h>

namespace core {

struct DynamicStackType {
	int a;
	int b;
};

TEST(DynamicStackTest, testPush) {
	core::DynamicStack<DynamicStackType> stack;
	stack.push(DynamicStackType{1, 1});
	EXPECT_EQ(1, stack.pop().b);
}

} // namespace core
