/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Algorithm.h"
#include "core/collection/Array.h"

namespace core {

TEST(AlgorithmTest, testSort) {
	core::Array<int, 8> foo{{1, 5, 3, 7, 8, 10, 100, -100}};
	core::sort(foo.begin(), foo.end(), core::Less<int>());
	EXPECT_EQ(-100, foo[0]);
	EXPECT_EQ(1, foo[1]);
	EXPECT_EQ(3, foo[2]);
	EXPECT_EQ(5, foo[3]);
	EXPECT_EQ(7, foo[4]);
	EXPECT_EQ(8, foo[5]);
	EXPECT_EQ(10, foo[6]);
	EXPECT_EQ(100, foo[7]);
}

TEST(AlgorithmTest, testSort1) {
	core::Array<int, 1> foo{{1}};
	core::sort(foo.begin(), foo.end(), core::Less<int>());
	EXPECT_EQ(1, foo[0]);
}

TEST(AlgorithmTest, testSort2) {
	core::Array<int, 2> foo{{2, 1}};
	core::sort(foo.begin(), foo.end(), core::Less<int>());
	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(2, foo[1]);
}

TEST(AlgorithmTest, testEmpty) {
	core::Array<int, 0> foo{};
	core::sort(foo.begin(), foo.end(), core::Less<int>());
}

}
