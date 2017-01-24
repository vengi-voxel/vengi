/**
 * @file
 */

#include "voxel/tests/AbstractVoxelTest.h"
#include "voxel/OctreeVolume.h"

namespace voxel {

class OctreeTest: public AbstractVoxelTest {
};

TEST_F(OctreeTest, DISABLED_testOctreeVolume) {
	const glm::ivec3 mins(0, 0, 0);
	const glm::ivec3 maxs(1023, 127, 1023);
	const Region region(mins, maxs);
	OctreeVolume octreeVolume(&_volData, region, 128);
	octreeVolume.update(region.getCentre(), 1.0f);
}

}
