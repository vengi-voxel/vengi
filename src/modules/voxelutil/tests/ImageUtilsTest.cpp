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
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"
#include "voxelutil/ImportFace.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class ImageUtilsTest : public app::AbstractTest {
protected:
	int countVoxels(const voxel::RawVolume &volume) {
		return voxelutil::visitVolume(volume, voxelutil::EmptyVisitor(), voxelutil::SkipEmpty());
	}

	void validateVoxel(const voxel::RawVolume &volume, const palette::Palette &palette, const image::ImagePtr &image,
					   int x, int y) {
		const core::RGBA expectedColor = image->colorAt(x, y);
		const voxel::Voxel &voxel = volume.voxel(x, y, 0);
		const core::RGBA actualColor = palette.color(voxel.getColor());
		EXPECT_LT(core::Color::getDistance(expectedColor, actualColor, core::Color::Distance::HSB), 0.04f)
			<< "Expected color: " << core::Color::print(expectedColor)
			<< ", but got: " << core::Color::print(actualColor) << " for voxel at (" << x << ", " << y << ")";
	}

	void validateHeightmap(voxel::Voxel underground) {
		const core::String filename = "test-colored-heightmap.png";
		image::ImagePtr image = image::loadImage(filename);
		ASSERT_TRUE(image && image->isLoaded()) << "Failed to load image: " << filename;
		ASSERT_EQ(16, image->width()) << "Expected height to be 16, but got: " << image->height();
		ASSERT_EQ(16, image->height()) << "Expected width to be 16, but got: " << image->width();
		voxel::Region region(0, 0, 0, image->width() - 1, 31, image->height() - 1);
		voxel::RawVolume volume(region);
		voxel::RawVolumeWrapper wrapper(&volume);
		palette::Palette palette;
		palette.nippon();
		const int minHeight = 1;
		voxelutil::importColoredHeightmap(wrapper, palette, image, underground, 1, true);
		int minsY;
		if (voxel::isAir(underground.getMaterial())) {
			minsY = region.getLowerY() + 3;
		} else {
			minsY = region.getLowerY();
		}
		voxel::Region expectedRegion(region.getLowerX(), minsY, region.getLowerZ(), region.getUpperX(), 17,
									 region.getUpperZ());
		int expectedVoxelCount = voxel::isAir(underground.getMaterial()) ? image->width() * image->height() : 3626;
		EXPECT_EQ(expectedVoxelCount, countVoxels(volume));
		for (int x = 0; x < image->width(); ++x) {
			for (int z = 0; z < image->height(); ++z) {
				const core::RGBA expectedColor = image->colorAt(x, z);
				const int volumeHeight = region.getHeightInVoxels();
				const int expectedY = getHeightValueFromAlpha(expectedColor.a, true, volumeHeight, minHeight) - 1;
				const voxel::Voxel &voxel = volume.voxel(x, expectedY, z);
				const core::RGBA actualColor = palette.color(voxel.getColor());
				ASSERT_LT(core::Color::getDistance(expectedColor, actualColor, core::Color::Distance::HSB), 0.04f)
					<< "Expected color: " << core::Color::print(expectedColor)
					<< ", but got: " << core::Color::print(actualColor) << " for voxel at (" << x << ", "
					<< expectedY << ", " << z << ") with height alpha value " << (int)expectedColor.a
					<< " and min height " << minHeight << " and height in voxels " << volumeHeight;
					if (expectedY > 0) {
						const voxel::Voxel &actualUnderground = volume.voxel(x, expectedY - 1, z);
						EXPECT_EQ(underground.getMaterial(), actualUnderground.getMaterial())
							<< "Expected underground voxel at (" << x << ", " << expectedY - 1 << ", " << z
							<< ") to have material " << underground.getMaterial()
							<< ", but got: " << actualUnderground.getMaterial();
					}
			}
		}
	}
};

TEST_F(ImageUtilsTest, testImportAsPlane) {
	const image::ImagePtr &img = image::loadImage("test-palette-in.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	const int depth = 2;
	palette::Palette palette;
	palette.nippon();
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsPlane(img, palette, 2));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
	EXPECT_EQ(3758, countVoxels(*volume));
}

TEST_F(ImageUtilsTest, testImportAsVolumeBothSided) {
	const image::ImagePtr &img = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	int depth = 10;
	palette::Palette palette;
	palette.nippon();
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsVolume(img, palette, depth, true));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth * 2 + 1, volume->depth());
	EXPECT_EQ(40, countVoxels(*volume));
}

TEST_F(ImageUtilsTest, testImportAsVolumeSingleSided) {
	const image::ImagePtr &img = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	int depth = 9;
	palette::Palette palette;
	palette.nippon();
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsVolume(img, palette, depth, false));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
	EXPECT_EQ(28, countVoxels(*volume));
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
	palette::Palette palette;
	palette.nippon();
	voxel::FaceNames faceName = voxel::FaceNames::PositiveZ;
	ASSERT_TRUE(
		voxelutil::importFace(volume, region, palette, faceName, image, glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f)));
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

TEST_F(ImageUtilsTest, testGetHeightValueFromAlpha) {
	EXPECT_EQ(0, getHeightValueFromAlpha(0, true, 10, 0));
	EXPECT_EQ(5, getHeightValueFromAlpha(127, true, 10, 0));
	EXPECT_EQ(10, getHeightValueFromAlpha(255, true, 10, 0));
	EXPECT_EQ(1, getHeightValueFromAlpha(0, true, 10, 1));
	EXPECT_EQ(15, getHeightValueFromAlpha(127, true, 31, 0));
	EXPECT_EQ(17, getHeightValueFromAlpha(132, true, 32, 1));
}

TEST_F(ImageUtilsTest, testImportColoredHeightmap) {
	const voxel::Voxel underground = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	validateHeightmap(underground);
}

TEST_F(ImageUtilsTest, testImportColoredHeightmapSurfaceOnly) {
	const voxel::Voxel underground = voxel::createVoxel(voxel::VoxelType::Air, 1);
	validateHeightmap(underground);
}

} // namespace voxelutil
