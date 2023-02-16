/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "image/Image.h"
#include "core/tests/TestColorHelper.h"
#include "io/FileStream.h"
#include <glm/vec2.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace glm {
::std::ostream &operator<<(::std::ostream &os, const vec2 &v) {
	os << to_string(v);
	return os;
}
::std::ostream &operator<<(::std::ostream &os, const ivec2 &v) {
	os << to_string(v);
	return os;
}
}

namespace image {

class ImageTest : public app::AbstractTest {
};

TEST_F(ImageTest, DISABLED_testWriteJPEG) {
	const core::RGBA r = core::RGBA(255, 0, 0);
	const core::RGBA b = core::RGBA(0, 0, 0);
	const core::RGBA img1[]{
		r, r, b, b, b, b,
		r, r, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
	};
	const core::RGBA img2[]{
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		r, r, b, b, b, b,
		r, r, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
	};
	const core::RGBA img3[]{
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, r, r, b, b,
		b, b, r, r, b, b,
	};
	const io::FilePtr &jpeg1 = io::filesystem()->open("1.jpeg", io::FileMode::SysWrite);
	const io::FilePtr &jpeg2 = io::filesystem()->open("2.jpeg", io::FileMode::SysWrite);
	const io::FilePtr &jpeg3 = io::filesystem()->open("3.jpeg", io::FileMode::SysWrite);

	io::FileStream stream1(jpeg1);
	ASSERT_TRUE(stream1.valid());
	ASSERT_TRUE(image::Image::writeJPEG(stream1, (const uint8_t*)img1, 6, 6, 4));

	io::FileStream stream2(jpeg2);
	ASSERT_TRUE(stream2.valid());
	ASSERT_TRUE(image::Image::writeJPEG(stream2, (const uint8_t*)img2, 6, 6, 4));

	io::FileStream stream3(jpeg3);
	ASSERT_TRUE(stream3.valid());
	ASSERT_TRUE(image::Image::writeJPEG(stream3, (const uint8_t*)img3, 6, 6, 4));
}

TEST_F(ImageTest, testGet) {
	const image::ImagePtr& img = image::loadImage("test-palette-in.png");
	const core::RGBA rgba = img->colorAt(33, 7);
	const core::RGBA expected(243, 238, 236);
	EXPECT_EQ(rgba, expected) << image::print(img);
}

TEST_F(ImageTest, testUVPixelBoundaries) {
	const image::ImagePtr& img = image::loadImage("test-palette-in.png");
	ASSERT_EQ(glm::vec2(0.0f, 0.0f), img->uv(0, img->height())) << "lower left corner of the image";
	ASSERT_EQ(glm::vec2(1.0f, 0.0f), img->uv(img->width(), img->height())) << "lower right corner of the image";
	ASSERT_EQ(glm::vec2(1.0f, 1.0f), img->uv(img->width(), 0)) << "upper right corner of the image";
	ASSERT_EQ(glm::vec2(0.0f, 1.0f), img->uv(0, 0)) << "upper left corner of the image";
}

TEST_F(ImageTest, testUVPixelConversion) {
	const image::ImagePtr& img = image::loadImage("test-palette-in.png");
	for (int x = 0; x < img->width(); ++x) {
		for (int y = 0; y < img->height(); ++y) {
			const glm::vec2 &uv = img->uv(x, y);
			const glm::ivec2 &pixels = img->pixels(uv);
			ASSERT_EQ(x, pixels.x) << "Failed to convert " << x << ":" << y << " to uv and back to pixels " << uv
								   << " image: " << img->width() << "," << img->height();
			ASSERT_EQ(y, pixels.y) << "Failed to convert " << x << ":" << y << " to uv and back to pixels " << uv
								   << " image: " << img->width() << "," << img->height();
		}
	}
}

}
