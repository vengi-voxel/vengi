/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Algorithm.h"
#include "core/ArrayLength.h"
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
	core::Array<int, 2> foo{{0, -1}};
	core::sort(foo.begin(), foo.begin(), core::Less<int>());
	EXPECT_EQ(0, foo[0]);
	EXPECT_EQ(-1, foo[1]);
}

TEST(AlgorithmTest, testPartially) {
	core::Array<int, 5> foo{{0, -1, -2, -4, -6}};
	core::sort(foo.begin(), core::next(foo.begin(), 2), core::Less<int>());
	EXPECT_EQ(-1, foo[0]);
	EXPECT_EQ( 0, foo[1]);
	EXPECT_EQ(-2, foo[2]);
	EXPECT_EQ(-4, foo[3]);
	EXPECT_EQ(-6, foo[4]);
}

TEST(AlgorithmTest, testNext) {
	core::Array<int, 5> foo{{0, -1, -2, -4, -6}};
	auto iter = foo.begin();
	EXPECT_EQ(iter, core::next(foo.begin(), 0));
	++iter;
	EXPECT_EQ(iter, core::next(foo.begin(), 1));
}

TEST(AlgorithmTest, testDistance) {
	core::Array<int, 5> foo{{0, -1, -2, -4, -6}};
	EXPECT_EQ((int)foo.size(), core::distance(foo.begin(), foo.end()));
}

TEST(AlgorithmTest, sortedDifference) {
	int out[8];
	{
		const int buf1[] = {1, 2, 3, 4, 5};
		const int buf2[] = {5, 6, 7, 8, 9};
		int amount = 0;
		core::sortedDifference(buf1, lengthof(buf1), buf2, lengthof(buf2), out, lengthof(out), amount);
		ASSERT_EQ(4, amount);
		EXPECT_EQ(1, out[0]);
		EXPECT_EQ(2, out[1]);
		EXPECT_EQ(3, out[2]);
		EXPECT_EQ(4, out[3]);
	}
}

}
