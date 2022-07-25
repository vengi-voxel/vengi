/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "image/Image.h"

namespace image {

class ImageTest : public app::AbstractTest {
};

TEST_F(ImageTest, testGet) {
	const image::ImagePtr& img = image::loadImage("test-palette-in.png", false);
	const core::RGBA rgba = img->colorAt(33, 7);
	const core::RGBA expected(243, 238, 236);
	EXPECT_EQ(rgba, expected);
}

}
