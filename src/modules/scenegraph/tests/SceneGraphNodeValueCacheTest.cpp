/**
 * @file
 */

#include "scenegraph/SceneGraphNodeValueCache.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Region.h"
#include <gtest/gtest.h>

namespace voxedit {

class SceneGraphNodeValueCacheTest : public testing::Test {};

TEST_F(SceneGraphNodeValueCacheTest, testInitialState) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	EXPECT_FALSE(cache.valid(InvalidNodeId));
	EXPECT_EQ(cache.size(), 0);
}

TEST_F(SceneGraphNodeValueCacheTest, testSetAndQuery) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	const voxel::Region region(0, 0, 0, 10, 10, 10);
	cache.set(42, region);
	EXPECT_TRUE(cache.valid(42));
	ASSERT_NE(cache.value(42), nullptr);
	EXPECT_EQ(*cache.value(42), region);
}

TEST_F(SceneGraphNodeValueCacheTest, testInvalidateAll) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	cache.set(42, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(42));
	cache.invalidate();
	EXPECT_FALSE(cache.valid(42));
}

TEST_F(SceneGraphNodeValueCacheTest, testInvalidateByNodeId) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	cache.set(42, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(42));

	// wrong node id - should not invalidate
	cache.invalidate(99);
	EXPECT_TRUE(cache.valid(42));

	// correct node id - should invalidate
	cache.invalidate(42);
	EXPECT_FALSE(cache.valid(42));
}

TEST_F(SceneGraphNodeValueCacheTest, testOverwrite) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	cache.set(1, voxel::Region(0, 5));
	cache.set(2, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(2));
	ASSERT_NE(cache.value(2), nullptr);
	EXPECT_EQ(*cache.value(2), voxel::Region(0, 10));
}

TEST_F(SceneGraphNodeValueCacheTest, testMultipleNodes) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	const voxel::Region region1(0, 5);
	const voxel::Region region2(0, 10);
	const voxel::Region region3(0, 15);
	cache.set(1, region1);
	cache.set(2, region2);
	cache.set(3, region3);
	EXPECT_EQ(cache.size(), 3);
	EXPECT_TRUE(cache.valid(1));
	EXPECT_TRUE(cache.valid(2));
	EXPECT_TRUE(cache.valid(3));
	EXPECT_EQ(*cache.value(1), region1);
	EXPECT_EQ(*cache.value(2), region2);
	EXPECT_EQ(*cache.value(3), region3);

	// invalidate one node - others remain
	cache.invalidate(2);
	EXPECT_TRUE(cache.valid(1));
	EXPECT_FALSE(cache.valid(2));
	EXPECT_TRUE(cache.valid(3));
	EXPECT_EQ(cache.size(), 2);
}

} // namespace voxedit
