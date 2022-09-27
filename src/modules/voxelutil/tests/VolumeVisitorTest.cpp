/**
 * @file
 */

#include "voxelutil/VolumeVisitor.h"
#include "app/tests/AbstractTest.h"

namespace voxelutil {

class VolumeVisitorTest : public app::AbstractTest {};

TEST_F(VolumeVisitorTest, testVisitSurface) {
	const voxel::Region region(0, 31);
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

} // namespace voxelutil
