/**
 * @file
 */

#include "voxelutil/ImageUtils.h"
#include "app/tests/AbstractTest.h"
#include "core/Color.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/tests/TestColorHelper.h"
#include "image/Image.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class ImageUtilsTest : public app::AbstractTest {
protected:
	int countVoxels(const voxel::RawVolume &volume) {
		return voxelutil::visitVolume(volume, voxelutil::EmptyVisitor(), voxelutil::SkipEmpty());
	}

	void validateVoxel(const voxel::RawVolume &volume, const palette::Palette &palette, const image::ImagePtr &image, int x, int y) {
		const core::RGBA expectedColor = image->colorAt(x, y);
		const voxel::Voxel &voxel = volume.voxel(x, y, 0);
		const core::RGBA actualColor = palette.color(voxel.getColor());
		EXPECT_LT(core::Color::getDistance(expectedColor, actualColor, core::Color::Distance::HSB), 0.04f)
			<< "Expected color: " << core::Color::print(expectedColor) << ", but got: "
			<< core::Color::print(actualColor) << " for voxel at (" << x << ", " << y << ")";
	}
};

TEST_F(ImageUtilsTest, testImportAsPlane) {
	const image::ImagePtr &img = image::loadImage("test-palette-in.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	const int depth = 2;
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsPlane(img, 2));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
}

TEST_F(ImageUtilsTest, testImportAsVolume) {
	const image::ImagePtr &img = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	int depth = 10;
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsVolume(img, depth, true));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth * 2 + 1, volume->depth());
}

TEST_F(ImageUtilsTest, testImportAsVolume2) {
	const image::ImagePtr &img = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	int depth = 9;
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsVolume(img, depth, false));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
}

TEST_F(ImageUtilsTest, testImportHeightMaxHeightAlpha) {
	constexpr int h = 4;
	constexpr int w = 4;
	constexpr core::RGBA buffer[w * h]{{255, 0, 0, 127},  {255, 255, 0, 128},  {255, 0, 255, 129},	{255, 255, 255, 1},
									   {0, 255, 0, 0},	  {13, 255, 50, 45},   {127, 127, 127, 45}, {255, 127, 0, 32},
									   {255, 0, 0, 45},	  {255, 60, 0, 45},	   {255, 0, 30, 45},	{127, 69, 255, 45},
									   {127, 127, 0, 45}, {255, 127, 127, 45}, {255, 0, 127, 45},	{0, 127, 80, 45}};
	static_assert(sizeof(buffer) == (size_t)w * (size_t)h * sizeof(uint32_t), "Unexpected rgba buffer size");
	const image::ImagePtr &texture = image::createEmptyImage("4x4");
	texture->loadRGBA((const uint8_t *)buffer, w, h);
	ASSERT_TRUE(texture);
	ASSERT_EQ(w, texture->width());
	ASSERT_EQ(h, texture->height());

	const int maxHeight = voxelutil::importHeightMaxHeight(texture, true);
	EXPECT_EQ(129, maxHeight);
}

TEST_F(ImageUtilsTest, testImportFace) {
	const image::ImagePtr &image = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(image && image->isLoaded());
	voxel::Region region(0, 0, 0, image->width() - 1, image->height() - 1, 0);
	voxel::RawVolume volume(region);
	voxel::RawVolumeWrapper wrapper(&volume);
	palette::Palette palette;
	palette.nippon();
	voxel::FaceNames faceName = voxel::FaceNames::PositiveZ;
	ASSERT_TRUE(
		voxelutil::importFace(wrapper, region, palette, faceName, image, glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f)));
	ASSERT_EQ(8, countVoxels(volume));

	validateVoxel(volume, palette, image, 5, 7);
	validateVoxel(volume, palette, image, 6, 5);
	validateVoxel(volume, palette, image, 6, 6);
	validateVoxel(volume, palette, image, 6, 7);
	validateVoxel(volume, palette, image, 7, 4);
	validateVoxel(volume, palette, image, 7, 5);
	validateVoxel(volume, palette, image, 7, 6);
	validateVoxel(volume, palette, image, 7, 7);
}

} // namespace voxelutil
