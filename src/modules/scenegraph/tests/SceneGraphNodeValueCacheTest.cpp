/**
 * @file
 */

#include "scenegraph/SceneGraphNodeValueCache.h"
#include "core/UUID.h"
#include "voxel/Region.h"
#include <gtest/gtest.h>

namespace voxedit {

class SceneGraphNodeValueCacheTest : public testing::Test {};

TEST_F(SceneGraphNodeValueCacheTest, testInitialState) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	EXPECT_FALSE(cache.valid(core::UUID()));
	EXPECT_EQ(cache.size(), 0);
}

TEST_F(SceneGraphNodeValueCacheTest, testSetAndQuery) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	const voxel::Region region(0, 0, 0, 10, 10, 10);
	const core::UUID uuid(42);
	cache.set(uuid, region);
	EXPECT_TRUE(cache.valid(uuid));
	ASSERT_NE(cache.value(uuid), nullptr);
	EXPECT_EQ(*cache.value(uuid), region);
}

TEST_F(SceneGraphNodeValueCacheTest, testInvalidateAll) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	const core::UUID uuid(42);
	cache.set(uuid, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(uuid));
	cache.invalidate();
	EXPECT_FALSE(cache.valid(uuid));
}

TEST_F(SceneGraphNodeValueCacheTest, testInvalidateByNodeUUID) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	const core::UUID uuid(42);
	cache.set(uuid, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(uuid));

	// wrong uuid - should not invalidate
	cache.invalidate(core::UUID(99));
	EXPECT_TRUE(cache.valid(uuid));

	// correct uuid - should invalidate
	cache.invalidate(uuid);
	EXPECT_FALSE(cache.valid(uuid));
}

TEST_F(SceneGraphNodeValueCacheTest, testOverwrite) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	const core::UUID uuid1(1);
	const core::UUID uuid2(2);
	cache.set(uuid1, voxel::Region(0, 5));
	cache.set(uuid2, voxel::Region(0, 10));
	EXPECT_TRUE(cache.valid(uuid2));
	ASSERT_NE(cache.value(uuid2), nullptr);
	EXPECT_EQ(*cache.value(uuid2), voxel::Region(0, 10));
}

TEST_F(SceneGraphNodeValueCacheTest, testMultipleNodes) {
	SceneGraphNodeValueCache<voxel::Region> cache;
	const voxel::Region region1(0, 5);
	const voxel::Region region2(0, 10);
	const voxel::Region region3(0, 15);
	const core::UUID uuid1(1);
	const core::UUID uuid2(2);
	const core::UUID uuid3(3);
	cache.set(uuid1, region1);
	cache.set(uuid2, region2);
	cache.set(uuid3, region3);
	EXPECT_EQ(cache.size(), 3);
	EXPECT_TRUE(cache.valid(uuid1));
	EXPECT_TRUE(cache.valid(uuid2));
	EXPECT_TRUE(cache.valid(uuid3));
	EXPECT_EQ(*cache.value(uuid1), region1);
	EXPECT_EQ(*cache.value(uuid2), region2);
	EXPECT_EQ(*cache.value(uuid3), region3);

	// invalidate one node - others remain
	cache.invalidate(uuid2);
	EXPECT_TRUE(cache.valid(uuid1));
	EXPECT_FALSE(cache.valid(uuid2));
	EXPECT_TRUE(cache.valid(uuid3));
	EXPECT_EQ(cache.size(), 2);
}

} // namespace voxedit
