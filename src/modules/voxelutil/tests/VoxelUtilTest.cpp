/**
 * @file
 */

#include "voxelutil/VoxelUtil.h"
#include "app/tests/AbstractTest.h"
#include "voxel/Face.h"
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
	voxel::RawVolumeWrapper wrapper(&v);
	voxelutil::fillHollow(wrapper, fillVoxel);
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
	voxel::RawVolumeWrapper wrapper(&v);
	voxelutil::fillHollow(wrapper, fillVoxel);
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
	voxel::RawVolumeWrapper wrapper(&v);
	voxelutil::fillHollow(wrapper, fillVoxel);
	EXPECT_EQ(0, v.voxel(region.getCenter()).getColor());
}

TEST_F(VoxelUtilTest, testExtrudePlanePositiveY) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel groundVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	const voxel::Voxel newPlaneVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 3);
	voxel::RawVolumeWrapper wrapper(&v);
	// build an L
	v.setVoxel(0, 0, 0, groundVoxel);
	v.setVoxel(1, 0, 0, groundVoxel);
	v.setVoxel(2, 0, 0, groundVoxel);
	v.setVoxel(2, 0, 1, groundVoxel);
	EXPECT_EQ(4, voxelutil::extrudePlane(wrapper, glm::ivec3(1, 1, 0), voxel::FaceNames::PositiveY, groundVoxel, newPlaneVoxel));
	EXPECT_EQ(8, voxelutil::visitVolume(v, [&](int, int, int, const voxel::Voxel &) {}));
}

TEST_F(VoxelUtilTest, testPaintPlanePositiveY) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel fillVoxel1 = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	const voxel::Voxel fillVoxel2 = voxel::createVoxel(voxel::VoxelType::Generic, 3);
	voxel::RawVolumeWrapper wrapper(&v);
	// build an L
	v.setVoxel(0, 0, 0, fillVoxel1); // first group
	v.setVoxel(1, 0, 0, fillVoxel1); // first group and selected for the paint call
	v.setVoxel(2, 0, 0, fillVoxel2); // second group here is the plane split
	v.setVoxel(2, 0, 1, fillVoxel1); // second group
	EXPECT_EQ(2, voxelutil::paintPlane(wrapper, glm::ivec3(1, 0, 0), voxel::FaceNames::PositiveY, fillVoxel1, fillVoxel2));
	int voxel2counter = 0;
	EXPECT_EQ(4, voxelutil::visitVolume(v, [&](int, int, int, const voxel::Voxel &voxel) {if (voxel.getColor() == fillVoxel2.getColor()) voxel2counter++;}));
	EXPECT_EQ(3, voxel2counter);
}

TEST_F(VoxelUtilTest, testErasePlanePositiveY) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel fillVoxel1 = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	const voxel::Voxel fillVoxel2 = voxel::createVoxel(voxel::VoxelType::Generic, 3);
	voxel::RawVolumeWrapper wrapper(&v);
	// build an L
	v.setVoxel(0, 0, 0, fillVoxel1); // first group
	v.setVoxel(1, 0, 0, fillVoxel1); // first group and selected for the erase call
	v.setVoxel(2, 0, 0, fillVoxel2); // second group here is the plane split
	v.setVoxel(2, 0, 1, fillVoxel1); // second group
	EXPECT_EQ(2, voxelutil::erasePlane(wrapper, glm::ivec3(1, 0, 0), voxel::FaceNames::PositiveY, fillVoxel1));
	EXPECT_EQ(2, voxelutil::visitVolume(v, [&](int, int, int, const voxel::Voxel &) {}));
}

TEST_F(VoxelUtilTest, testFillEmptyPlaneNegativeX) {
	voxel::Region region(-2, 0);
	voxel::RawVolume v(region);
	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxel::RawVolumeWrapper wrapper(&v);
	EXPECT_EQ(9, voxelutil::extrudePlane(wrapper, glm::ivec3(0, -1, -1), voxel::FaceNames::NegativeX, voxel::Voxel(), fillVoxel));
	EXPECT_EQ(9, voxelutil::visitVolume(v, [&](int, int, int, const voxel::Voxel &) {}));
}

TEST_F(VoxelUtilTest, testFillEmptyPlanePositiveY) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxel::RawVolumeWrapper wrapper(&v);
	EXPECT_EQ(9, voxelutil::extrudePlane(wrapper, glm::ivec3(1, 0, 1), voxel::FaceNames::PositiveY, voxel::Voxel(), fillVoxel));
	EXPECT_EQ(9, voxelutil::visitVolume(v, [&](int, int, int, const voxel::Voxel &) {}));
}

TEST_F(VoxelUtilTest, testFillEmptyPlanePositiveZ) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxel::RawVolumeWrapper wrapper(&v);
	EXPECT_EQ(9, voxelutil::extrudePlane(wrapper, glm::ivec3(1, 1, 0), voxel::FaceNames::PositiveZ, voxel::Voxel(), fillVoxel));
	EXPECT_EQ(9, voxelutil::visitVolume(v, [&](int, int, int, const voxel::Voxel &) {}));
}

TEST_F(VoxelUtilTest, DISABLED_testFillPlaneWithImage) {
	// TODO: implement me voxelutil::fillPlane
}

} // namespace voxelutil
