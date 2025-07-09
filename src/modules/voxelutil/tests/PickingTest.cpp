/**
 * @file
 */

#include "voxelutil/Picking.h"
#include "app/tests/AbstractTest.h"
#include "core/GLM.h"
#include "math/tests/TestMathHelper.h"
#include "voxel/RawVolume.h"

namespace voxelutil {

class PickingTest : public app::AbstractTest {};

TEST_F(PickingTest, testPicking) {
	voxel::RawVolume v(voxel::Region(glm::ivec3(0), glm::ivec3(10)));
	ASSERT_TRUE(v.setVoxel(glm::ivec3(0), voxel::createVoxel(voxel::VoxelType::Generic, 0)));
	const glm::vec3 start(0.0f, 3.0f, 0.0f);
	const glm::vec3 direction = glm::down() * 100.0f;
	const PickResult &result = pickVoxel(&v, start, direction, voxel::Voxel());
	ASSERT_TRUE(result.didHit) << "Expected to hit the voxel at (0, 0, 0)";
	ASSERT_EQ(glm::ivec3(0), result.hitVoxel)
		<< "Expected to hit the voxel at (0, 0, 0) - but got " << result.hitVoxel.x << ", " << result.hitVoxel.y << ", "
		<< result.hitVoxel.z;
	ASSERT_TRUE(result.validPreviousPosition);
	ASSERT_EQ(glm::ivec3(0, 1, 0), result.previousPosition)
		<< "Expected to get a previous position of (0, 1, 0) - but got " << result.previousPosition.x << ", "
		<< result.previousPosition.y << ", " << result.previousPosition.z;
}

} // namespace voxelutil
