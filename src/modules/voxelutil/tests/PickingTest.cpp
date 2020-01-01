/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxelutil/Picking.h"
#include "core/GLM.h"

namespace voxel {

class PickingTest: public core::AbstractTest {
};

TEST_F(PickingTest, testPicking) {
	RawVolume v(Region(glm::ivec3(0), glm::ivec3(10)));
	v.setVoxel(glm::ivec3(0), createVoxel(VoxelType::Grass, 0));
	const PickResult& result = pickVoxel(&v, glm::vec3(0.0f, 3.0f, 0.0f), glm::down * 100.0f, voxel::Voxel());
	ASSERT_TRUE(result.didHit);
	ASSERT_EQ(glm::ivec3(0), result.hitVoxel);
	ASSERT_EQ(glm::ivec3(0, 1, 0), result.previousPosition);
}

}
