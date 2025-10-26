/**
 * @file
 */

#include "voxelutil/VoxelUtil.h"
#include "app/tests/AbstractTest.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"
#include "voxelutil/FillHollow.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class VoxelUtilTest : public app::AbstractTest {
};

TEST_F(VoxelUtilTest, testFillHollow3x3Center) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel borderVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	auto func = [&](int x, int y, int z, const voxel::Voxel &) { v.setVoxel(x, y, z, borderVoxel); };
	voxelutil::visitVolumeParallel(v, func, VisitAll());
	EXPECT_EQ(voxelutil::countVoxelsByType(v, borderVoxel), region.voxels());
	EXPECT_TRUE(v.setVoxel(region.getCenter(), voxel::Voxel()));
	EXPECT_EQ(voxelutil::countVoxelsByType(v, borderVoxel), region.voxels() - 1);

	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxelutil::fillHollow(v, fillVoxel);
	EXPECT_EQ(fillVoxel.getColor(), v.voxel(region.getCenter()).getColor());
	EXPECT_EQ(voxelutil::countVoxelsByColor(v, borderVoxel), region.voxels() - 1);
	EXPECT_EQ(voxelutil::countVoxelsByColor(v, fillVoxel), 1);
}

TEST_F(VoxelUtilTest, testFillHollow5x5CenterNegativeOrigin) {
	voxel::Region region(-2, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel borderVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	auto func = [&](int x, int y, int z, const voxel::Voxel &) { v.setVoxel(x, y, z, borderVoxel); };
	voxelutil::visitVolumeParallel(v, func, VisitAll());
	EXPECT_TRUE(v.setVoxel(region.getCenter(), voxel::Voxel()));

	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxelutil::fillHollow(v, fillVoxel);
	EXPECT_EQ(2, v.voxel(region.getCenter()).getColor());
}

TEST_F(VoxelUtilTest, testFillHollowLeak) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel borderVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	auto func = [&](int x, int y, int z, const voxel::Voxel &) { v.setVoxel(x, y, z, borderVoxel); };
	voxelutil::visitVolumeParallel(v, func, VisitAll());
	EXPECT_TRUE(v.setVoxel(region.getCenter(), voxel::Voxel()));
	EXPECT_TRUE(v.setVoxel(1, 1, 0, voxel::Voxel())); // produce leak

	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxelutil::fillHollow(v, fillVoxel);
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
	EXPECT_EQ(4, voxelutil::extrudePlane(wrapper, glm::ivec3(1, 1, 0), voxel::FaceNames::PositiveY, groundVoxel,
										 newPlaneVoxel, 1));
	EXPECT_EQ(8, voxelutil::countVoxels(v));
}

TEST_F(VoxelUtilTest, testOverridePlanePositiveY) {
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
	EXPECT_EQ(9, voxelutil::overridePlane(wrapper, glm::ivec3(1, 0, 0), voxel::FaceNames::PositiveY, newPlaneVoxel));
	EXPECT_EQ(9, voxelutil::countVoxelsByColor(v, newPlaneVoxel));
}

TEST_F(VoxelUtilTest, testExtrudePlaneRegion) {
	voxel::Region region(0, 7);
	voxel::RawVolume v(region);
	const voxel::Voxel groundVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	const voxel::Voxel newPlaneVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 3);
	voxel::RawVolumeWrapper wrapper(&v);
	// build an L
	v.setVoxel(0, 0, 0, groundVoxel);
	v.setVoxel(1, 0, 0, groundVoxel);
	v.setVoxel(2, 0, 0, groundVoxel);
	v.setVoxel(2, 0, 1, groundVoxel);
	const int thickness = 5;
	const voxel::Region extrudeRegion = voxelutil::extrudePlaneRegion(
		*wrapper.volume(), glm::ivec3(1, 1, 0), voxel::FaceNames::PositiveY, groundVoxel, newPlaneVoxel, thickness);
	EXPECT_EQ(thickness, extrudeRegion.getDimensionsInVoxels().y)
		<< extrudeRegion.getDimensionsInVoxels().x << ":" << extrudeRegion.getDimensionsInVoxels().y << ":"
		<< extrudeRegion.getDimensionsInVoxels().z;
}

TEST_F(VoxelUtilTest, testExtrudeEraseRegion) {
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
	const voxel::Region extrudeRegion =
		voxelutil::erasePlaneRegion(*wrapper.volume(), glm::ivec3(1, 0, 0), voxel::FaceNames::PositiveY, fillVoxel1);
	EXPECT_EQ(1, extrudeRegion.getDimensionsInVoxels().y);
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
	EXPECT_EQ(2,
			  voxelutil::paintPlane(wrapper, glm::ivec3(1, 0, 0), voxel::FaceNames::PositiveY, fillVoxel1, fillVoxel2));
	int voxel2counter = 0;
	auto func = [&](int, int, int, const voxel::Voxel &voxel) {
		if (voxel.getColor() == fillVoxel2.getColor())
			voxel2counter++;
	};
	EXPECT_EQ(4, voxelutil::visitVolume(v, func));
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
	EXPECT_EQ(2, voxelutil::countVoxels(v));
}

TEST_F(VoxelUtilTest, testFillEmptyPlaneNegativeX) {
	voxel::Region region(-2, 0);
	voxel::RawVolume v(region);
	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxel::RawVolumeWrapper wrapper(&v);
	EXPECT_EQ(9, voxelutil::extrudePlane(wrapper, glm::ivec3(0, -1, -1), voxel::FaceNames::NegativeX, voxel::Voxel(),
										 fillVoxel, 1));
	EXPECT_EQ(9, voxelutil::countVoxels(v));
}

TEST_F(VoxelUtilTest, testFillEmptyPlanePositiveY) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxel::RawVolumeWrapper wrapper(&v);
	EXPECT_EQ(9, voxelutil::extrudePlane(wrapper, glm::ivec3(1, 0, 1), voxel::FaceNames::PositiveY, voxel::Voxel(),
										 fillVoxel, 1));
	EXPECT_EQ(9, voxelutil::countVoxels(v));
}

TEST_F(VoxelUtilTest, testFillEmptyPlanePositiveZ) {
	voxel::Region region(0, 2);
	voxel::RawVolume v(region);
	const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	voxel::RawVolumeWrapper wrapper(&v);
	EXPECT_EQ(9, voxelutil::extrudePlane(wrapper, glm::ivec3(1, 1, 0), voxel::FaceNames::PositiveZ, voxel::Voxel(),
										 fillVoxel, 1));
	EXPECT_EQ(9, voxelutil::countVoxels(v));
}

TEST_F(VoxelUtilTest, testFillPlaneWithImage) {
	palette::Palette pal;
	pal.nippon();
	palette::PaletteLookup palLookup(pal);

	const image::ImagePtr &img = image::loadImage("test-fillplane.png");

	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	EXPECT_EQ(10, img->width());
	EXPECT_EQ(3, img->height());

	voxel::Region region(0, 0, 0, img->width() - 1, img->height() - 1, 1);
	EXPECT_EQ(region.getHeightInVoxels(), img->height());
	EXPECT_EQ(region.getWidthInVoxels(), img->width());
	voxel::RawVolume v(region);
	voxel::RawVolumeWrapper wrapper(&v);
	const int plane1Voxels =
		voxelutil::fillPlane(wrapper, img, voxel::Voxel(), glm::ivec3(0, 0, 0), voxel::FaceNames::PositiveZ);
	EXPECT_EQ(img->width() * img->height(), plane1Voxels);

	for (int x = 0; x < img->width(); ++x) {
		const core::RGBA rgba = img->colorAt(x, 0);
		const voxel::Voxel &voxel = wrapper.voxel(x, region.getHeightInCells(), 0);
		const core::RGBA voxelColor = palLookup.palette().color(voxel.getColor());
		EXPECT_EQ(core::Color::getDistance(rgba, voxelColor, core::Color::Distance::HSB), 0.0f)
			<< core::Color::print(rgba) << " vs " << core::Color::print(voxelColor) << " (" << (int)voxel.getColor()
			<< ") at " << x << "\n"
			<< v;
	}

	const int plane2Voxels =
		voxelutil::fillPlane(wrapper, img, voxel::Voxel(), glm::ivec3(0, 0, 1), voxel::FaceNames::PositiveZ);
	EXPECT_EQ(img->width() * img->height(), plane2Voxels);

	for (int x = 0; x < img->width(); ++x) {
		const core::RGBA rgba = img->colorAt(x, 0);
		const voxel::Voxel &voxel = wrapper.voxel(x, region.getHeightInCells(), 1);
		const core::RGBA voxelColor = palLookup.palette().color(voxel.getColor());
		EXPECT_EQ(core::Color::getDistance(rgba, voxelColor, core::Color::Distance::HSB), 0.0f)
			<< core::Color::print(rgba) << " vs " << core::Color::print(voxelColor) << " (" << (int)voxel.getColor()
			<< ") at " << x << "\n"
			<< v;
	}
}

TEST_F(VoxelUtilTest, copyIntoRegion) {
	voxel::RawVolume v{voxel::Region{-20, 20}};
	v.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	v.setVoxel(v.region().getLowerCorner(), voxel::createVoxel(voxel::VoxelType::Generic, 2));
	v.setVoxel(v.region().getUpperCorner(), voxel::createVoxel(voxel::VoxelType::Generic, 3));
	voxel::RawVolume out(voxel::Region{-40, 40});
	ASSERT_TRUE(out.copyInto(v));
	EXPECT_EQ(1, out.voxel(0, 0, 0).getColor());
	EXPECT_EQ(2, out.voxel(v.region().getLowerCorner()).getColor());
	EXPECT_EQ(3, out.voxel(v.region().getUpperCorner()).getColor());
}

} // namespace voxelutil
