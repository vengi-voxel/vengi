/**
 * @file
 */

#include "voxelutil/VolumeResizer.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class VolumeResizerTest : public app::AbstractTest {};

TEST_F(VolumeResizerTest, testResize) {
	voxel::RawVolume volume({-8, 8});
	voxelutil::visitVolumeParallel(volume, [&](int x, int y, int z, const voxel::Voxel &voxel) {
		volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	}, VisitAll());
	voxel::Region newRegion = volume.region();
	newRegion.grow(5);
	core::ScopedPtr<voxel::RawVolume> v(voxelutil::resize(&volume, newRegion));
	ASSERT_TRUE(v);
	ASSERT_TRUE(voxel::isBlocked(v->voxel(volume.region().getLowerCorner()).getMaterial()))	;
	ASSERT_FALSE(voxel::isBlocked(v->voxel(volume.region().getUpperCorner() + 1).getMaterial()))	;
}

} // namespace voxelutil
