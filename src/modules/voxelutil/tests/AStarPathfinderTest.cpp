/**
 * @file
 */

#include "voxelutil/AStarPathfinder.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"

namespace voxelutil {

class AStarPathfinderTest : public app::AbstractTest {
protected:
	static bool standOnSolid(const voxel::RawVolume *v, const glm::ivec3 &pos) {
		const glm::ivec3 below(pos.x, pos.y - 1, pos.z);
		return voxel::isBlocked(v->voxel(below).getMaterial());
	}
};

TEST_F(AStarPathfinderTest, testGroundPath) {
	voxel::RawVolume volume(voxel::Region(0, 20));
	for (int x = 0; x < 20; ++x) {
		for (int z = 0; z < 20; ++z) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	const glm::ivec3 start(0, 1, 0);
	const glm::ivec3 end(10, 1, 19);
	core::List<glm::ivec3> listResult;

	AStarPathfinderParams<voxel::RawVolume> params(&volume, start, end, &listResult,
												   [](const voxel::RawVolume *v, const glm::ivec3 &pos) {
													   return standOnSolid(v, pos);
												   });
	AStarPathfinder pathfinder(params);
	EXPECT_TRUE(pathfinder.execute());
	EXPECT_EQ(20u, listResult.size());
	ASSERT_FALSE(listResult.empty());
	EXPECT_EQ(start, *listResult.begin());
	ASSERT_NE(nullptr, listResult.back());
	EXPECT_EQ(end, *listResult.back());
}

TEST_F(AStarPathfinderTest, testObstacleForcesDetour) {
	voxel::RawVolume volume(voxel::Region(0, 15));
	for (int x = 0; x <= 15; ++x) {
		for (int z = 0; z <= 15; ++z) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	// Wall of air "holes" in the walkable layer along x=7 (no ground below -> invalid)
	for (int z = 0; z <= 15; ++z) {
		volume.setVoxel(7, 0, z, voxel::Voxel());
	}
	// Leave a gap at z=14 so a path still exists around the wall
	volume.setVoxel(7, 0, 14, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	core::List<glm::ivec3> listResult;
	AStarPathfinderParams<voxel::RawVolume> params(
		&volume, glm::ivec3(0, 1, 0), glm::ivec3(14, 1, 0), &listResult,
		[](const voxel::RawVolume *v, const glm::ivec3 &pos) { return standOnSolid(v, pos); }, 1.0f, 10000,
		voxel::Connectivity::SixConnected);
	AStarPathfinder pathfinder(params);
	ASSERT_TRUE(pathfinder.execute());
	EXPECT_GE(listResult.size(), 15u);
	bool crossedGap = false;
	for (const glm::ivec3 &p : listResult) {
		EXPECT_TRUE(standOnSolid(&volume, p));
		if (p.x == 7) {
			crossedGap = true;
			EXPECT_EQ(p.z, 14);
		}
	}
	EXPECT_TRUE(crossedGap);
}

TEST_F(AStarPathfinderTest, testUnreachableReturnsFalse) {
	voxel::RawVolume volume(voxel::Region(0, 10));
	for (int x = 0; x <= 4; ++x) {
		for (int z = 0; z <= 10; ++z) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	for (int x = 6; x <= 10; ++x) {
		for (int z = 0; z <= 10; ++z) {
			volume.setVoxel(x, 0, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
		}
	}
	core::List<glm::ivec3> listResult;
	AStarPathfinderParams<voxel::RawVolume> params(
		&volume, glm::ivec3(0, 1, 0), glm::ivec3(10, 1, 0), &listResult,
		[](const voxel::RawVolume *v, const glm::ivec3 &pos) { return standOnSolid(v, pos); }, 1.0f, 10000,
		voxel::Connectivity::SixConnected);
	AStarPathfinder pathfinder(params);
	EXPECT_FALSE(pathfinder.execute());
	EXPECT_TRUE(listResult.empty());
}

} // namespace voxelutil
