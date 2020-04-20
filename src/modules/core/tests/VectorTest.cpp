/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/collection/Vector.h"

namespace core {

TEST(VectorTest, testEmpty) {
	core::Vector<int, 3> foo;
	EXPECT_EQ(0u, foo.size());
}

TEST(VectorTest, testFill) {
	core::Vector<int, 3> foo;
	foo.fill(42);
	EXPECT_EQ(3u, foo.size());
	EXPECT_EQ(42, foo[0]);
	EXPECT_EQ(42, foo[1]);
	EXPECT_EQ(42, foo[2]);
}

TEST(VectorTest, testIterate) {
	core::Vector<int, 30> foo;
	foo.fill(42);
	EXPECT_EQ(30u, foo.size());
	for (int v : foo) {
		EXPECT_EQ(42, v);
	}
}

TEST(VectorTest, testPushBack) {
	core::Vector<int, 3> foo;
	EXPECT_EQ(0u, foo.size());
	EXPECT_EQ(3u, foo.capacity());
	foo.push_back(1);
	EXPECT_EQ(1u, foo.size());
	foo.push_back(2);
	EXPECT_EQ(2u, foo.size());
	foo.push_back(3);
	EXPECT_EQ(3u, foo.size());
}

}
