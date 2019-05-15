/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "../tool/ImageUtils.h"
#include "voxel/MaterialColor.h"

namespace voxedit {

class ImageUtilsTest: public core::AbstractTest {
};

TEST_F(ImageUtilsTest, testCreateAndLoadPalette) {
	const image::ImagePtr& img = image::loadImage("test-palette-in.png", false);
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	uint32_t buf[256];
	EXPECT_TRUE(voxel::createPalette(img, buf, 256)) << "Failed to create palette image";
	EXPECT_TRUE(voxel::overrideMaterialColors((const uint8_t*)buf, sizeof(buf), ""));
}

}
