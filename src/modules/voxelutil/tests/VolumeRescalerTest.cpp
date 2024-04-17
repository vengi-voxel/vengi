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
		voxelutil::visitVolume(volume, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		});
		core::ScopedPtr<voxel::RawVolume> v(voxelutil::scaleUp(volume));
		ASSERT_TRUE(v);
		const voxel::Region scaledRegion = v->region();
		const glm::ivec3 dims = scaledRegion.getDimensionsInVoxels();
		const glm::ivec3 mins = scaledRegion.getLowerCorner();
		ASSERT_EQ(dims, volume.region().getDimensionsInVoxels() * 2);
		ASSERT_EQ(mins, volume.region().getLowerCorner());
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

} // namespace voxelutil
