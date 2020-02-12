/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/Stack.h"

namespace core {

struct StackType {
	int a;
	int b;
};

TEST(StackTest, testPush) {
	core::Stack<StackType, 8> stack;
	stack.push(StackType{1, 1});
	EXPECT_EQ(1, stack.pop().b);
}

}
