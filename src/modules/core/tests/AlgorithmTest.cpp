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
		const int buf1[] = {1, 2, 3, 4, 5, 7, 10, 11, 12};
		const int buf2[] = {5, 6, 7, 8, 9, 10, 11, 13};
		int amount = 0;
		core::sortedDifference(buf1, lengthof(buf1), buf2, lengthof(buf2), out, lengthof(out), amount);
		ASSERT_EQ(5, amount);
		EXPECT_EQ(1, out[0]);
		EXPECT_EQ(2, out[1]);
		EXPECT_EQ(3, out[2]);
		EXPECT_EQ(4, out[3]);
		EXPECT_EQ(12, out[4]);
		amount = 0;
		core::sortedDifference(buf2, lengthof(buf2), buf1, lengthof(buf1), out, lengthof(out), amount);
		ASSERT_EQ(4, amount);
		EXPECT_EQ(6, out[0]);
		EXPECT_EQ(8, out[1]);
		EXPECT_EQ(9, out[2]);
		EXPECT_EQ(13, out[3]);
	}
}

TEST(AlgorithmTest, sortedIntersection) {
	int out[4];
	{
		const int buf1[] = {1, 2, 3, 4, 5, 10, 11, 12, 19, 21, 23, 26};
		const int buf2[] = {5, 6, 7, 8, 9, 10, 13, 15, 19, 24, 25, 26};
		int amount = 0;
		core::sortedIntersection(buf1, lengthof(buf1), buf2, lengthof(buf2), out, lengthof(out), amount);
		ASSERT_GE(4, amount);
		EXPECT_EQ(4, amount);
		EXPECT_EQ(5, out[0]);
		EXPECT_EQ(10, out[1]);
		EXPECT_EQ(19, out[2]);
		EXPECT_EQ(26, out[3]);
		amount = 0;
		core::sortedIntersection(buf2, lengthof(buf2), buf1, lengthof(buf1), out, lengthof(out), amount);
		ASSERT_GE(4, amount);
		EXPECT_EQ(4, amount);
		EXPECT_EQ(5, out[0]);
		EXPECT_EQ(10, out[1]);
		EXPECT_EQ(19, out[2]);
		EXPECT_EQ(26, out[3]);
	}
}

TEST(AlgorithmTest, sortedUnion) {
	int out[16];
	{
		const int buf1[] = {1, 2, 3, 4, 5};
		const int buf2[] = {5, 6, 7, 8, 9};
		int amount = 0;
		core::sortedUnion(buf1, lengthof(buf1), buf2, lengthof(buf2), out, lengthof(out), amount);
		ASSERT_EQ(9, amount);
		EXPECT_EQ(1, out[0]);
		EXPECT_EQ(2, out[1]);
		EXPECT_EQ(3, out[2]);
		EXPECT_EQ(4, out[3]);
		EXPECT_EQ(5, out[4]);
		EXPECT_EQ(6, out[5]);
		EXPECT_EQ(7, out[6]);
		EXPECT_EQ(8, out[7]);
		EXPECT_EQ(9, out[8]);
		amount = 0;
		core::sortedUnion(buf2, lengthof(buf2), buf1, lengthof(buf1), out, lengthof(out), amount);
		ASSERT_EQ(9, amount);
		EXPECT_EQ(1, out[0]);
		EXPECT_EQ(2, out[1]);
		EXPECT_EQ(3, out[2]);
		EXPECT_EQ(4, out[3]);
		EXPECT_EQ(5, out[4]);
		EXPECT_EQ(6, out[5]);
		EXPECT_EQ(7, out[6]);
		EXPECT_EQ(8, out[7]);
		EXPECT_EQ(9, out[8]);
	}
}

TEST(AlgorithmTest, testLowerBound) {
	core::Array<int, 5> foo{{1, 3, 5, 7, 9}};

	// Value exists in array
	auto it = core::lower_bound(foo.begin(), foo.end(), 5, core::Less<int>());
	EXPECT_EQ(5, *it);
	EXPECT_EQ(2, core::distance(foo.begin(), it));

	// Value doesn't exist, between elements
	it = core::lower_bound(foo.begin(), foo.end(), 4, core::Less<int>());
	EXPECT_EQ(5, *it);
	EXPECT_EQ(2, core::distance(foo.begin(), it));

	// Value smaller than all
	it = core::lower_bound(foo.begin(), foo.end(), 0, core::Less<int>());
	EXPECT_EQ(1, *it);
	EXPECT_EQ(0, core::distance(foo.begin(), it));

	// Value larger than all
	it = core::lower_bound(foo.begin(), foo.end(), 10, core::Less<int>());
	EXPECT_EQ(foo.end(), it);
}

TEST(AlgorithmTest, testLowerBoundDuplicates) {
	core::Array<int, 7> foo{{1, 2, 2, 2, 5, 7, 9}};

	// Should find first occurrence
	auto it = core::lower_bound(foo.begin(), foo.end(), 2, core::Less<int>());
	EXPECT_EQ(2, *it);
	EXPECT_EQ(1, core::distance(foo.begin(), it));
}

TEST(AlgorithmTest, testLowerBoundEmpty) {
	core::Array<int, 5> foo{{1, 2, 3, 4, 5}};
	auto it = core::lower_bound(foo.begin(), foo.begin(), 3, core::Less<int>());
	EXPECT_EQ(foo.begin(), it);
}

TEST(AlgorithmTest, testUpperBound) {
	core::Array<int, 5> foo{{1, 3, 5, 7, 9}};

	// Value exists in array
	auto it = core::upper_bound(foo.begin(), foo.end(), 5, core::Less<int>());
	EXPECT_EQ(7, *it);
	EXPECT_EQ(3, core::distance(foo.begin(), it));

	// Value doesn't exist, between elements
	it = core::upper_bound(foo.begin(), foo.end(), 4, core::Less<int>());
	EXPECT_EQ(5, *it);
	EXPECT_EQ(2, core::distance(foo.begin(), it));

	// Value smaller than all
	it = core::upper_bound(foo.begin(), foo.end(), 0, core::Less<int>());
	EXPECT_EQ(1, *it);
	EXPECT_EQ(0, core::distance(foo.begin(), it));

	// Value larger than all
	it = core::upper_bound(foo.begin(), foo.end(), 10, core::Less<int>());
	EXPECT_EQ(foo.end(), it);
}

TEST(AlgorithmTest, testUpperBoundDuplicates) {
	core::Array<int, 7> foo{{1, 2, 2, 2, 5, 7, 9}};

	// Should find position after last occurrence
	auto it = core::upper_bound(foo.begin(), foo.end(), 2, core::Less<int>());
	EXPECT_EQ(5, *it);
	EXPECT_EQ(4, core::distance(foo.begin(), it));
}

TEST(AlgorithmTest, testRotateForward) {
	core::Array<int, 6> foo{{1, 2, 3, 4, 5, 6}};

	// Rotate [1,2,3,4,5,6] with middle at 4 -> [4,5,6,1,2,3]
	auto result = core::rotate_forward(foo.begin(), core::next(foo.begin(), 3), foo.end());

	EXPECT_EQ(4, foo[0]);
	EXPECT_EQ(5, foo[1]);
	EXPECT_EQ(6, foo[2]);
	EXPECT_EQ(1, foo[3]);
	EXPECT_EQ(2, foo[4]);
	EXPECT_EQ(3, foo[5]);
	EXPECT_EQ(3, core::distance(foo.begin(), result));
}

TEST(AlgorithmTest, testRotateForwardSmall) {
	core::Array<int, 3> foo{{1, 2, 3}};

	// Rotate [1,2,3] with middle at 2 -> [2,3,1]
	core::rotate_forward(foo.begin(), core::next(foo.begin(), 1), foo.end());

	EXPECT_EQ(2, foo[0]);
	EXPECT_EQ(3, foo[1]);
	EXPECT_EQ(1, foo[2]);
}

TEST(AlgorithmTest, testRotateForwardEdgeCases) {
	// Test with first == middle
	core::Array<int, 3> foo1{{1, 2, 3}};
	auto result1 = core::rotate_forward(foo1.begin(), foo1.begin(), foo1.end());
	EXPECT_EQ(foo1.end(), result1);
	EXPECT_EQ(1, foo1[0]);
	EXPECT_EQ(2, foo1[1]);
	EXPECT_EQ(3, foo1[2]);

	// Test with middle == last
	core::Array<int, 3> foo2{{1, 2, 3}};
	auto result2 = core::rotate_forward(foo2.begin(), foo2.end(), foo2.end());
	EXPECT_EQ(foo2.begin(), result2);
	EXPECT_EQ(1, foo2[0]);
	EXPECT_EQ(2, foo2[1]);
	EXPECT_EQ(3, foo2[2]);
}

TEST(AlgorithmTest, testInplaceMergeBasic) {
	core::Array<int, 6> foo{{1, 3, 5, 2, 4, 6}};

	// Merge [1,3,5] and [2,4,6]
	core::inplace_merge(foo.begin(), core::next(foo.begin(), 3), foo.end(), core::Less<int>());

	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(2, foo[1]);
	EXPECT_EQ(3, foo[2]);
	EXPECT_EQ(4, foo[3]);
	EXPECT_EQ(5, foo[4]);
	EXPECT_EQ(6, foo[5]);
}

TEST(AlgorithmTest, testInplaceMergeAlreadySorted) {
	core::Array<int, 6> foo{{1, 2, 3, 4, 5, 6}};

	core::inplace_merge(foo.begin(), core::next(foo.begin(), 3), foo.end(), core::Less<int>());

	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(2, foo[1]);
	EXPECT_EQ(3, foo[2]);
	EXPECT_EQ(4, foo[3]);
	EXPECT_EQ(5, foo[4]);
	EXPECT_EQ(6, foo[5]);
}

TEST(AlgorithmTest, testInplaceMergeReversed) {
	core::Array<int, 6> foo{{4, 5, 6, 1, 2, 3}};

	// Merge [4,5,6] and [1,2,3]
	core::inplace_merge(foo.begin(), core::next(foo.begin(), 3), foo.end(), core::Less<int>());

	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(2, foo[1]);
	EXPECT_EQ(3, foo[2]);
	EXPECT_EQ(4, foo[3]);
	EXPECT_EQ(5, foo[4]);
	EXPECT_EQ(6, foo[5]);
}

TEST(AlgorithmTest, testInplaceMergeUnequalSizes) {
	core::Array<int, 7> foo{{1, 5, 9, 2, 3, 4, 7}};

	// Merge [1,5,9] and [2,3,4,7]
	core::inplace_merge(foo.begin(), core::next(foo.begin(), 3), foo.end(), core::Less<int>());

	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(2, foo[1]);
	EXPECT_EQ(3, foo[2]);
	EXPECT_EQ(4, foo[3]);
	EXPECT_EQ(5, foo[4]);
	EXPECT_EQ(7, foo[5]);
	EXPECT_EQ(9, foo[6]);
}

TEST(AlgorithmTest, testInplaceMergeDuplicates) {
	core::Array<int, 8> foo{{1, 3, 5, 5, 2, 3, 5, 6}};

	// Merge [1,3,5,5] and [2,3,5,6]
	core::inplace_merge(foo.begin(), core::next(foo.begin(), 4), foo.end(), core::Less<int>());

	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(2, foo[1]);
	EXPECT_EQ(3, foo[2]);
	EXPECT_EQ(3, foo[3]);
	EXPECT_EQ(5, foo[4]);
	EXPECT_EQ(5, foo[5]);
	EXPECT_EQ(5, foo[6]);
	EXPECT_EQ(6, foo[7]);
}

TEST(AlgorithmTest, testInplaceMergeSmall) {
	core::Array<int, 2> foo{{2, 1}};

	// Merge [2] and [1]
	core::inplace_merge(foo.begin(), core::next(foo.begin(), 1), foo.end(), core::Less<int>());

	EXPECT_EQ(1, foo[0]);
	EXPECT_EQ(2, foo[1]);
}

TEST(AlgorithmTest, testInplaceMergeLarge) {
	// Test with larger array to exercise the divide-and-conquer path
	core::Array<int, 64> foo;

	// Fill first half: even numbers 0-62
	for (int i = 0; i < 32; ++i) {
		foo[i] = i * 2;
	}

	// Fill second half: odd numbers 1-63
	for (int i = 0; i < 32; ++i) {
		foo[32 + i] = i * 2 + 1;
	}

	core::inplace_merge(foo.begin(), core::next(foo.begin(), 32), foo.end(), core::Less<int>());

	for (int i = 0; i < 64; ++i) {
		EXPECT_EQ(i, foo[i]) << "at index " << i;
	}
}

TEST(AlgorithmTest, testInplaceMergeNegatives) {
	core::Array<int, 6> foo{{-5, -1, 3, -4, -2, 0}};

	// Merge [-5,-1,3] and [-4,-2,0]
	core::inplace_merge(foo.begin(), core::next(foo.begin(), 3), foo.end(), core::Less<int>());

	EXPECT_EQ(-5, foo[0]);
	EXPECT_EQ(-4, foo[1]);
	EXPECT_EQ(-2, foo[2]);
	EXPECT_EQ(-1, foo[3]);
	EXPECT_EQ(0, foo[4]);
	EXPECT_EQ(3, foo[5]);
}

TEST(AlgorithmTest, testInplaceMergeEmptyRanges) {
	// Test with empty first range
	core::Array<int, 3> foo1{{1, 2, 3}};
	core::inplace_merge(foo1.begin(), foo1.begin(), foo1.end(), core::Less<int>());
	EXPECT_EQ(1, foo1[0]);
	EXPECT_EQ(2, foo1[1]);
	EXPECT_EQ(3, foo1[2]);

	// Test with empty second range
	core::Array<int, 3> foo2{{1, 2, 3}};
	core::inplace_merge(foo2.begin(), foo2.end(), foo2.end(), core::Less<int>());
	EXPECT_EQ(1, foo2[0]);
	EXPECT_EQ(2, foo2[1]);
	EXPECT_EQ(3, foo2[2]);
}

TEST(AlgorithmTest, testInplaceMergeGreater) {
	// Test with Greater comparator
	core::Array<int, 6> foo{{6, 4, 2, 5, 3, 1}};

	// Merge [6,4,2] and [5,3,1] in descending order
	core::inplace_merge(foo.begin(), core::next(foo.begin(), 3), foo.end(), core::Greater<int>());

	EXPECT_EQ(6, foo[0]);
	EXPECT_EQ(5, foo[1]);
	EXPECT_EQ(4, foo[2]);
	EXPECT_EQ(3, foo[3]);
	EXPECT_EQ(2, foo[4]);
	EXPECT_EQ(1, foo[5]);
}

TEST(AlgorithmTest, testInplaceMergeMultipleChunks) {
	// Simulate what the parallel sort does - sort chunks then merge them
	constexpr int size = 64;
	core::Array<int, size> foo;

	// Fill with pseudo-random data similar to AsyncTest
	for (int i = 0; i < size; ++i) {
		foo[i] = (i * 13 + 7) % 100;
	}

	// Sort in chunks of 16 (simulating parallel sort chunks)
	constexpr int chunkSize = 16;
	for (int i = 0; i < size; i += chunkSize) {
		core::sort(foo.begin() + i, foo.begin() + core_min(i + chunkSize, size), core::Less<int>());
	}

	// Now merge the chunks
	int currentChunkSize = chunkSize;
	while (currentChunkSize < size) {
		const int mergeSize = currentChunkSize * 2;
		for (int i = 0; i + currentChunkSize < size; i += mergeSize) {
			auto leftFirst = foo.begin() + i;
			auto middle = foo.begin() + core_min(i + currentChunkSize, size);
			auto rightLast = foo.begin() + core_min(i + mergeSize, size);
			core::inplace_merge(leftFirst, middle, rightLast, core::Less<int>());
		}
		currentChunkSize = mergeSize;
	}

	for (int i = 1; i < size; ++i) {
		EXPECT_LE(foo[i - 1], foo[i]) << " at index " << i;
	}
}

} // namespace core
