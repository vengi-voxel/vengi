/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxelutil/VolumeMover.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxelutil {

class VolumeMoverTest: public app::AbstractTest {
};


TEST_F(VolumeMoverTest, testMove) {
	voxel::Region regionBig = voxel::Region(0, 5);
	voxel::RawVolume bigVolume(regionBig);
	ASSERT_TRUE(bigVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(bigVolume.setVoxel(1, 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1)));

	voxel::RawVolume newVolume(bigVolume.region());
	voxel::RawVolumeMoveWrapper wrapper(&newVolume);
	glm::ivec3 offsets(1, 0, 0);
	voxelutil::moveVolume(&wrapper, &bigVolume, offsets);

	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(1, 0, 0).getMaterial())) << "Expected to find a voxel at (1, 0, 0)\n";
	EXPECT_TRUE(voxel::isBlocked(wrapper.voxel(2, 1, 0).getMaterial())) << "Expected to find a voxel at (2, 1, 0)\n";
}

TEST_F(VolumeMoverTest, testMoveDenseCount) {
	// Regression: dest Z used to stay on the first plane, so only one slice was copied.
	voxel::RawVolume source(voxel::Region(0, 7));
	source.fill(voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume dest(voxel::Region(0, 7));
	const int moved = voxelutil::moveVolume(&dest, &source, glm::ivec3(0, 0, 0));
	EXPECT_EQ(source.region().voxels(), moved);
	EXPECT_EQ(source.region().voxels(),
			  voxelutil::visitVolumeParallel(dest, voxelutil::EmptyVisitor(), voxelutil::VisitSolid()));
}

TEST_F(VolumeMoverTest, testMoveWithZOffset) {
	voxel::RawVolume source(voxel::Region(0, 3));
	source.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume dest(voxel::Region(0, 3));
	ASSERT_EQ(1, voxelutil::moveVolume(&dest, &source, glm::ivec3(0, 0, 1)));
	EXPECT_TRUE(voxel::isAir(dest.voxel(1, 1, 1).getMaterial()));
	EXPECT_TRUE(voxel::isBlocked(dest.voxel(1, 1, 2).getMaterial()));
}

} // namespace voxelutil
