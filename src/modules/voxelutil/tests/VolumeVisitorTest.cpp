/**
 * @file
 */

#include "voxelutil/VolumeVisitor.h"
#include "app/tests/AbstractTest.h"

namespace voxelutil {

class VolumeVisitorTest : public app::AbstractTest {};

TEST_F(VolumeVisitorTest, testVisitSurface) {
	const voxel::Region region(0, 2);
	const voxel::Voxel voxel = createVoxel(voxel::VoxelType::Generic, 1);
	voxel::RawVolume volume(region);
	for (int x = 0; x < 3; ++x) {
		for (int y = 0; y < 3; ++y) {
			for (int z = 0; z < 3; ++z) {
				volume.setVoxel(x, y, z, voxel);
			}
		}
	}

	int cnt = visitSurfaceVolume(
		volume, [&](int x, int y, int z, const voxel::Voxel &voxel) {}, VisitorOrder::XZY);
	EXPECT_EQ(26, cnt);
}

TEST_F(VolumeVisitorTest, testVisitSurfaceCorners) {
	const voxel::Region region(0, 2);
	const voxel::Voxel voxel = createVoxel(voxel::VoxelType::Generic, 1);
	voxel::RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(0, 0, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(0, 0, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(0, 2, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 2, 2, voxel));

	EXPECT_TRUE(volume.setVoxel(0, 2, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 0, 0, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 0, 2, voxel));
	EXPECT_TRUE(volume.setVoxel(2, 2, 0, voxel));

	int cnt = visitSurfaceVolume(
		volume, [&](int x, int y, int z, const voxel::Voxel &voxel) {}, VisitorOrder::XZY);
	EXPECT_EQ(8, cnt);
}

} // namespace voxelutil
