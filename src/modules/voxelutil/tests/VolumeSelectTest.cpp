/**
 * @file
 */

#include "voxelutil/VolumeSelect.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "math/Vec3i.h"

namespace voxelutil {

class VolumeSelectTest : public app::AbstractTest {};

static void fillBox(voxel::RawVolume &volume, const voxel::Region &r, const voxel::Voxel &v) {
	const math::Vec3i &lo = r.getLowerCorner();
	const math::Vec3i &hi = r.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				volume.setVoxel(x, y, z, v);
			}
		}
	}
}

TEST_F(VolumeSelectTest, testLineMarkSolidDiagonal) {
	voxel::Region region(-5, 5);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillBox(volume, voxel::Region(math::Vec3i(-2, -2, -2), math::Vec3i(2, 2, 2)), solid);

	int marked = 0;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) {
		EXPECT_EQ(x, y);
		EXPECT_EQ(y, z);
		++marked;
	};
	lineMarkSolid(volume, math::Vec3i(-2, -2, -2), math::Vec3i(2, 2, 2), 1, region, markFunc);
	EXPECT_EQ(marked, 5);
}

TEST_F(VolumeSelectTest, testLineDrapeSurfaceAcrossStep) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = 0; x <= 10; ++x) {
		const int top = x <= 5 ? 10 : 9;
		for (int z = 0; z <= 2; ++z) {
			for (int y = 0; y <= top; ++y) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	core::DynamicSet<math::Vec3i, 257, math::Vec3iHash> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(math::Vec3i(x, y, z)); };

	lineDrapeSurface(volume, math::Vec3i(0, 10, 1), math::Vec3i(10, 9, 1), 0, 2, 1, true, 1, region, markFunc);

	EXPECT_TRUE(selected.has(math::Vec3i(5, 10, 1)));
	EXPECT_TRUE(selected.has(math::Vec3i(6, 9, 1)));
	EXPECT_TRUE(selected.has(math::Vec3i(10, 9, 1)));
	EXPECT_FALSE(selected.has(math::Vec3i(5, 5, 1)));
}

} // namespace voxelutil
