/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxelutil/ImageUtils.h"
#include "voxel/MaterialColor.h"
#include "core/ScopedPtr.h"
#include "voxel/Palette.h"

namespace voxelutil {

class ImageUtilsTest: public app::AbstractTest {
};

TEST_F(ImageUtilsTest, testImportAsPlane) {
	const image::ImagePtr& img = image::loadImage("test-palette-in.png", false);
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	const int depth = 2;
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsPlane(img, 2));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
}

}
