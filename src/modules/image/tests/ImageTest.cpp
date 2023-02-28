/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "image/Image.h"
#include "core/tests/TestColorHelper.h"
#include "io/BufferedReadWriteStream.h"
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
private:
	static constexpr core::RGBA r = core::RGBA(255, 0, 0);
	static constexpr core::RGBA b = core::RGBA(0, 0, 0);
protected:
	static constexpr core::RGBA img1[]{
		r, r, b, b, b, b,
		r, r, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
	};
	static constexpr core::RGBA img2[]{
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		r, r, b, b, b, b,
		r, r, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
	};
	static constexpr core::RGBA img3[]{
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, r, r, b, b,
		b, b, r, r, b, b,
	};
};

TEST_F(ImageTest, testWriteJPEG) {
	io::BufferedReadWriteStream stream1;
	ASSERT_TRUE(image::Image::writeJPEG(stream1, (const uint8_t*)img1, 6, 6, 4));
	stream1.seek(0);
	ASSERT_TRUE(image::createEmptyImage("image")->load(stream1, stream1.size()));

	io::BufferedReadWriteStream stream2;
	ASSERT_TRUE(image::Image::writeJPEG(stream2, (const uint8_t*)img2, 6, 6, 4));
	stream2.seek(0);
	ASSERT_TRUE(image::createEmptyImage("image")->load(stream2, stream1.size()));

	io::BufferedReadWriteStream stream3;
	ASSERT_TRUE(image::Image::writeJPEG(stream3, (const uint8_t*)img3, 6, 6, 4));
	stream3.seek(0);
	ASSERT_TRUE(image::createEmptyImage("image")->load(stream3, stream1.size()));
}

TEST_F(ImageTest, testWritePng) {
	io::BufferedReadWriteStream stream1;
	ASSERT_TRUE(image::Image::writePng(stream1, (const uint8_t*)img1, 6, 6, 4));
	stream1.seek(0);
	ASSERT_TRUE(image::createEmptyImage("image")->load(stream1, stream1.size()));

	io::BufferedReadWriteStream stream2;
	ASSERT_TRUE(image::Image::writePng(stream2, (const uint8_t*)img2, 6, 6, 4));
	stream2.seek(0);
	ASSERT_TRUE(image::createEmptyImage("image")->load(stream2, stream1.size()));

	io::BufferedReadWriteStream stream3;
	ASSERT_TRUE(image::Image::writePng(stream3, (const uint8_t*)img3, 6, 6, 4));
	stream3.seek(0);
	ASSERT_TRUE(image::createEmptyImage("image")->load(stream3, stream1.size()));
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
