/**
 * @file
 */

#include "voxelutil/Hollow.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class HollowTest : public app::AbstractTest {};

TEST_F(HollowTest, testHollow1RemoveCenter) {
	voxel::Region region(0, 2);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	// create a solid cube
	for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}
	voxelutil::hollow(volume);
	const int solidVoxels = visitVolumeParallel(volume, EmptyVisitor(), VisitSolid());
	const int cubeVoxels = region.voxels();
	EXPECT_LT(solidVoxels, cubeVoxels);
	EXPECT_EQ(cubeVoxels - 1, solidVoxels);
}

TEST_F(HollowTest, testHollow4RemoveCenter) {
	voxel::Region region(0, 3);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	// create a solid cube
	for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}
	voxelutil::hollow(volume);
	const int solidVoxels = visitVolumeParallel(volume, EmptyVisitor(), VisitSolid());
	const int cubeVoxels = region.voxels();
	EXPECT_LT(solidVoxels, cubeVoxels);
	EXPECT_EQ(cubeVoxels - 8, solidVoxels);
}

TEST_F(HollowTest, testHollow27RemoveCenter) {
	voxel::Region region(0, 4);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	// create a solid cube
	for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}
	voxelutil::hollow(volume);
	const int solidVoxels = visitVolumeParallel(volume, EmptyVisitor(), VisitSolid());
	const int cubeVoxels = region.voxels();
	EXPECT_LT(solidVoxels, cubeVoxels);
	EXPECT_EQ(cubeVoxels - 27, solidVoxels);
}

} // namespace voxelutil
