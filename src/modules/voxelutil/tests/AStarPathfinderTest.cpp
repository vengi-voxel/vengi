/**
 * @file
 */

#include "voxelutil/AStarPathfinder.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"

namespace voxelutil {

class AStarPathfinderTest : public app::AbstractTest {};

TEST_F(AStarPathfinderTest, test) {
	voxel::RawVolume volume(voxel::Region(0, 20));
	for (int x = 0; x < 20; ++x) {
		for (int z = 0; z < 20; ++z) {
			volume.setVoxel(x, 0, z, createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	const glm::ivec3 start(0, 1, 0);
	const glm::ivec3 end(10, 1, 19);
	core::List<glm::ivec3> listResult;

	AStarPathfinderParams<voxel::RawVolume> params(&volume, start, end, &listResult,
												   [](const voxel::RawVolume *v, const glm::ivec3 &pos) {
													   const glm::ivec3 below(pos.x, pos.y - 1, pos.z);
													   return voxel::isBlocked(v->voxel(below).getMaterial());
												   });
	AStarPathfinder pathfinder(params);
	EXPECT_TRUE(pathfinder.execute());
	EXPECT_EQ(20u, listResult.size());
}

} // namespace voxelutil
