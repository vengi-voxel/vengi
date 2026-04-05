/**
 * @file
 */

#include "voxedit-util/SceneGraphNodeCachedValue.h"
#include "voxel/Region.h"
#include <gtest/gtest.h>

namespace voxedit {

class SceneGraphNodeCachedValueTest : public testing::Test {};

TEST_F(SceneGraphNodeCachedValueTest, testInitialState) {
	SceneGraphNodeCachedValue<voxel::Region> cache;
	EXPECT_FALSE(cache.valid(InvalidNodeId));
	EXPECT_EQ(cache.nodeId(), InvalidNodeId);
}

TEST_F(SceneGraphNodeCachedValueTest, testSetAndQuery) {
	SceneGraphNodeCachedValue<voxel::Region> cache;
	const voxel::Region region(0, 0, 0, 10, 10, 10);
	cache.set(42, region);
	EXPECT_TRUE(cache.valid(42));
	EXPECT_EQ(cache.nodeId(), 42);
	ASSERT_NE(cache.value(), nullptr);
	EXPECT_EQ(*cache.value(), region);
}

TEST_F(SceneGraphNodeCachedValueTest, testInvalidateAll) {
	SceneGraphNodeCachedValue<voxel::Region> cache;
	cache.set(42, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(42));
	cache.invalidate();
	EXPECT_FALSE(cache.valid(42));
}

TEST_F(SceneGraphNodeCachedValueTest, testInvalidateByNodeId) {
	SceneGraphNodeCachedValue<voxel::Region> cache;
	cache.set(42, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(42));

	// wrong node id - should not invalidate
	cache.invalidate(99);
	EXPECT_TRUE(cache.valid(42));

	// correct node id - should invalidate
	cache.invalidate(42);
	EXPECT_FALSE(cache.valid(42));
}

TEST_F(SceneGraphNodeCachedValueTest, testOverwrite) {
	SceneGraphNodeCachedValue<voxel::Region> cache;
	cache.set(1, voxel::Region(0, 5));
	cache.set(2, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(2));
	EXPECT_EQ(cache.nodeId(), 2);
	ASSERT_NE(cache.value(), nullptr);
	EXPECT_EQ(*cache.value(), voxel::Region(0, 10));
}

} // namespace voxedit
