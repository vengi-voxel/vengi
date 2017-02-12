/**
 * @file
 */

#include "voxel/tests/AbstractVoxelTest.h"
#include "voxel/OctreeVolume.h"

namespace voxel {

class OctreeTest: public AbstractVoxelTest {
};

TEST_F(OctreeTest, testOctreeVolume) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(31, 31, 31);
	const Region region(mins, maxs);
	OctreeVolume octreeVolume(&_volData, region, 16);
	octreeVolume.update(1l, region.getCentre(), 1.0f);
	Octree& octree = octreeVolume.octree();
	OctreeNode* rootNode = octree.getRootNode();
	EXPECT_TRUE(rootNode->isActive());
	EXPECT_TRUE(rootNode->isMeshUpToDate());
	EXPECT_FALSE(rootNode->isScheduledForUpdate());
	EXPECT_EQ(nullptr, rootNode->getParentNode());
	int cnt = 0;
	rootNode->visitExistingChildren([&] (uint8_t x, uint8_t y, uint8_t z, OctreeNode* children) {
		++cnt;
	});
	EXPECT_EQ(8, cnt);
}

}
