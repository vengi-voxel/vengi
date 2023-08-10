/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxelutil/ImageUtils.h"
#include "voxel/RawVolume.h"
#include "core/ScopedPtr.h"

namespace voxelutil {

class ImageUtilsTest: public app::AbstractTest {
};

TEST_F(ImageUtilsTest, testImportAsPlane) {
	const image::ImagePtr& img = image::loadImage("test-palette-in.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	const int depth = 2;
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsPlane(img, 2));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
}

TEST_F(ImageUtilsTest, testImportAsVolume) {
	const image::ImagePtr& img = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	int depth = 10;
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsVolume(img, depth, true));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth * 2 + 1, volume->depth());
}

TEST_F(ImageUtilsTest, testImportAsVolume2) {
	const image::ImagePtr& img = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	int depth = 9;
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsVolume(img, depth, false));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
}

}
