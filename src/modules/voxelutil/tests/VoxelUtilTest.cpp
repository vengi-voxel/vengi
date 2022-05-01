/**
 * @file
 */

#include "voxelutil/VoxelUtil.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class VoxelUtilTest : public app::AbstractTest {};

TEST_F(VoxelUtilTest, testFillHollow3x3Center) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel borderVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxelutil::visitVolume(
		v, [&](int x, int y, int z, const voxel::Voxel &) { EXPECT_TRUE(v.setVoxel(x, y, z, borderVoxel)); },
		VisitAll());
	EXPECT_TRUE(v.setVoxel(region.getCenter(), voxel::Voxel()));

	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxelutil::fillHollow(v, fillVoxel);
	EXPECT_EQ(2, v.voxel(region.getCenter()).getColor());
}

TEST_F(VoxelUtilTest, testFillHollow5x5CenterNegativeOrigin) {
	voxel::Region region(-2, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel borderVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxelutil::visitVolume(
		v, [&](int x, int y, int z, const voxel::Voxel &) { EXPECT_TRUE(v.setVoxel(x, y, z, borderVoxel)); },
		VisitAll());
	EXPECT_TRUE(v.setVoxel(region.getCenter(), voxel::Voxel()));

	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxelutil::fillHollow(v, fillVoxel);
	EXPECT_EQ(2, v.voxel(region.getCenter()).getColor());
}

TEST_F(VoxelUtilTest, testFillHollowLeak) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel borderVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxelutil::visitVolume(
		v, [&](int x, int y, int z, const voxel::Voxel &) { EXPECT_TRUE(v.setVoxel(x, y, z, borderVoxel)); },
		VisitAll());
	EXPECT_TRUE(v.setVoxel(region.getCenter(), voxel::Voxel()));
	EXPECT_TRUE(v.setVoxel(1, 1, 0, voxel::Voxel())); // produce leak

	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxelutil::fillHollow(v, fillVoxel);
	EXPECT_EQ(0, v.voxel(region.getCenter()).getColor());
}

} // namespace voxelutil
