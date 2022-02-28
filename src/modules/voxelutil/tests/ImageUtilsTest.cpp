/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxelutil/ImageUtils.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"

namespace voxedit {

class ImageUtilsTest: public app::AbstractTest {
};

TEST_F(ImageUtilsTest, testCreateAndLoadPalette) {
	const image::ImagePtr& img = image::loadImage("test-palette-in.png", false);
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	voxel::Palette palette;
	EXPECT_TRUE(voxel::Palette::createPalette(img, palette)) << "Failed to create palette image";
	EXPECT_TRUE(voxel::overridePalette(palette));
}

}
