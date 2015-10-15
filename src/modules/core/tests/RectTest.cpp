#include <gtest/gtest.h>
#include "core/Rect.h"

namespace core {

TEST(RectTest, testContains) {
	RectuInt aabb(0, 0, 100, 100);

	ASSERT_TRUE(aabb.contains(aabb));
	ASSERT_TRUE(aabb.contains(RectuInt(0, 0, 1, 1)));
	ASSERT_TRUE(aabb.contains(RectuInt(99, 99, 100, 100)));

	ASSERT_FALSE(aabb.contains(RectuInt(101, 101, 102, 102)));
	ASSERT_FALSE(aabb.contains(RectuInt(0, 0, 101, 101)));
	ASSERT_FALSE(aabb.contains(RectuInt(100, 100, 101, 101)));
	ASSERT_FALSE(aabb.contains(RectuInt(1, 100, 100, 101)));
}

TEST(RectTest, testIntersectswith) {
	RectuInt aabb(0, 0, 100, 100);
	ASSERT_TRUE(aabb.intersectsWith(aabb));
	ASSERT_TRUE(aabb.intersectsWith(RectuInt(0, 0, 1, 1)));
	ASSERT_TRUE(aabb.intersectsWith(RectuInt(99, 99, 100, 100)));
	ASSERT_TRUE(aabb.intersectsWith(RectuInt(0, 0, 101, 101)));
	ASSERT_TRUE(aabb.intersectsWith(RectuInt(1, 99, 100, 100)));

	ASSERT_FALSE(aabb.intersectsWith(RectuInt(101, 101, 102, 102)));
	ASSERT_FALSE(aabb.intersectsWith(RectuInt(100, 100, 101, 101)));
}

}
