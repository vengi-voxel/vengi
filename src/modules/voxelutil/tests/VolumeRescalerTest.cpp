/**
 * @file
 */

#include "voxelutil/VolumeRescaler.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class VolumeRescalerTest : public app::AbstractTest {
protected:
	void testScaleUpFull(int lower, int upper) {
		voxel::RawVolume volume({lower, upper});
		const int n = voxelutil::visitVolumeParallel(volume, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		}, VisitAll());
		ASSERT_GT(n, 0);
		core::ScopedPtr<voxel::RawVolume> v(voxelutil::scaleUp(volume));
		ASSERT_TRUE(v);
		const voxel::Region scaledRegion = v->region();
		EXPECT_EQ(v->region().voxels(), n * 8);
		const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
		const glm::ivec3 mins = scaledRegion.getLowerCorner();
		ASSERT_EQ(dims, volume.region().getDimensionsInVoxels() * 2);
		ASSERT_EQ(mins, volume.region().getLowerCorner());
		EXPECT_EQ(voxelutil::countVoxels(*v), n * 8) << "Expected " << n * 8 << " voxels, but got " << countVoxels(*v);
	}
};

TEST_F(VolumeRescalerTest, testScaleUpEmpty) {
	voxel::RawVolume volume({-8, 8});
	core::ScopedPtr<voxel::RawVolume> v(voxelutil::scaleUp(volume));
	ASSERT_TRUE(v);
	const voxel::Region scaledRegion = v->region();
	const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
	const glm::ivec3 mins = scaledRegion.getLowerCorner();
	ASSERT_EQ(dims, volume.region().getDimensionsInVoxels() * 2);
	ASSERT_EQ(mins, volume.region().getLowerCorner());
}

TEST_F(VolumeRescalerTest, testScaleUpFull) {
	testScaleUpFull(-8, 8);
	testScaleUpFull(7, 8);
}

TEST_F(VolumeRescalerTest, testScaleDown) {
	voxel::RawVolume volume({-8, 8});
	for (int y = -8; y <= 8; ++y) {
		for (int x = -2; x <= 2; ++x) {
			for (int z = -2; z <= 2; ++z) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, y + 8));
			}
		}
	}
	palette::Palette pal;
	pal.nippon();
	ASSERT_EQ(5 * 5 * 17, voxelutil::countVoxels(volume));
	const voxel::Region &srcRegion = volume.region();
	const glm::ivec3 &targetDimensionsHalf = (srcRegion.getDimensionsInVoxels() / 2) - 1;
	const voxel::Region destRegion(srcRegion.getLowerCorner(), srcRegion.getLowerCorner() + targetDimensionsHalf);
	voxel::RawVolume destVolume(destRegion);
	voxelutil::scaleDown(volume, pal, destVolume);
	ASSERT_EQ(32, voxelutil::countVoxels(destVolume));
}

} // namespace voxelutil
