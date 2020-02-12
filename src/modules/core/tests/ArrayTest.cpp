/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/Array.h"

namespace core {

TEST(ArrayTest, testAssign) {
	constexpr core::Array<int, 3> foo{{1, 2, 3}};
	EXPECT_EQ(3u, foo.size());
}

TEST(ArrayTest, testFill) {
	core::Array<int, 3> foo;
	foo.fill(42);
	EXPECT_EQ(3u, foo.size());
	EXPECT_EQ(42, foo[0]);
	EXPECT_EQ(42, foo[1]);
	EXPECT_EQ(42, foo[2]);
}

TEST(ArrayTest, testIterate) {
	core::Array<int, 30> foo;
	foo.fill(42);
	EXPECT_EQ(30u, foo.size());
	for (int v : foo) {
		EXPECT_EQ(42, v);
	}
}

}
