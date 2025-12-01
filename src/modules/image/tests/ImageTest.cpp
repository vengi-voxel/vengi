/**
 * @file
 */

#include "image/Image.h"
#include "app/tests/AbstractTest.h"
#include "core/tests/TestColorHelper.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/MemoryReadStream.h"
#include "math/tests/TestMathHelper.h"

namespace image {

class ImageTest : public app::AbstractTest {
protected:
	static constexpr color::RGBA r = color::RGBA(255, 0, 0);
	static constexpr color::RGBA b = color::RGBA(0, 0, 0);

	static constexpr color::RGBA img1[]{
		r, r, b, b, b, b, r, r, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,
	};
	static constexpr color::RGBA img2[]{
		b, b, b, b, b, b, b, b, b, b, b, b, r, r, b, b, b, b, r, r, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,
	};
	static constexpr color::RGBA img3[]{
		b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, r, r, b, b, b, b, r, r, b, b,
	};

	bool validate(const image::ImagePtr &image, const color::RGBA *data, int w, int h, int d) {
		if (image->width() != w || image->height() != h || image->components() != d) {
			return false;
		}
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const color::RGBA c = image->colorAt(x, y);
				if (c != data[y * w + x]) {
					Log::error("Color mismatch at %i, %i: %s vs %s", x, y, color::print(c).c_str(),
							   color::print(data[y * w + x]).c_str());
					return false;
				}
			}
		}
		return true;
	}
};

TEST_F(ImageTest, testWriteJPEG) {
	io::BufferedReadWriteStream stream1;
	ASSERT_TRUE(image::Image::writeJPEG(stream1, (const uint8_t *)img1, 6, 6, 4));
	stream1.seek(0);
	image::ImagePtr image1 = image::createEmptyImage("image1");
	ASSERT_TRUE(image1->load(ImageType::JPEG, stream1, (int)stream1.size()));
	// ASSERT_TRUE(validate(image1, img1, 6, 6, 4));

	io::BufferedReadWriteStream stream2;
	ASSERT_TRUE(image::Image::writeJPEG(stream2, (const uint8_t *)img2, 6, 6, 4));
	stream2.seek(0);
	image::ImagePtr image2 = image::createEmptyImage("image2");
	ASSERT_TRUE(image2->load(ImageType::JPEG, stream2, (int)stream2.size()));
	// ASSERT_TRUE(validate(image2, img2, 6, 6, 4));

	io::BufferedReadWriteStream stream3;
	ASSERT_TRUE(image::Image::writeJPEG(stream3, (const uint8_t *)img3, 6, 6, 4));
	stream3.seek(0);
	// auto detect the image type here
	image::ImagePtr image3 = image::createEmptyImage("image3");
	ASSERT_TRUE(image3->load(ImageType::Unknown, stream3, (int)stream3.size()));
	// ASSERT_TRUE(validate(image3, img3, 6, 6, 4));
}

TEST_F(ImageTest, testWritePng) {
	io::BufferedReadWriteStream stream1;
	ASSERT_TRUE(image::Image::writePNG(stream1, (const uint8_t *)img1, 6, 6, 4));
	stream1.seek(0);
	image::ImagePtr image1 = image::createEmptyImage("image1");
	ASSERT_TRUE(image1->load(ImageType::PNG, stream1, (int)stream1.size()));
	ASSERT_TRUE(validate(image1, img1, 6, 6, 4));

	io::BufferedReadWriteStream stream2;
	ASSERT_TRUE(image::Image::writePNG(stream2, (const uint8_t *)img2, 6, 6, 4));
	stream2.seek(0);
	image::ImagePtr image2 = image::createEmptyImage("image2");
	ASSERT_TRUE(image2->load(ImageType::PNG, stream2, (int)stream2.size()));
	ASSERT_TRUE(validate(image2, img2, 6, 6, 4));

	io::BufferedReadWriteStream stream3;
	ASSERT_TRUE(image::Image::writePNG(stream3, (const uint8_t *)img3, 6, 6, 4));
	stream3.seek(0);
	// auto detect the image type here
	image::ImagePtr image3 = image::createEmptyImage("image3");
	ASSERT_TRUE(image3->load(ImageType::Unknown, stream3, (int)stream3.size()));
	ASSERT_TRUE(validate(image3, img3, 6, 6, 4));
}

TEST_F(ImageTest, testGet) {
	io::FilePtr file = _testApp->filesystem()->open("test-palette-in.png");
	const image::ImagePtr &img = image::loadImage(file);
	const color::RGBA rgba = img->colorAt(33, 7);
	const color::RGBA expected(243, 238, 236);
	EXPECT_EQ(rgba, expected) << image::print(img);
}

TEST_F(ImageTest, testUV) {
	EXPECT_VEC_NEAR(glm::vec2(0.0f, 0.0f), image::Image::uv(0, 0, 256, 1, false), 0.000001f)
		<< "lower left corner of the image";
	EXPECT_VEC_NEAR(glm::vec2(1.0f, 1.0f), image::Image::uv(255, 0, 256, 2, false), 0.000001f)
		<< "lower left corner of the image";
	EXPECT_VEC_NEAR(glm::vec2(1.0f, 0.0f), image::Image::uv(255, 1, 256, 2, false), 0.000001f)
		<< "lower left corner of the image";

	EXPECT_VEC_NEAR(glm::vec2(0.0f, 0.6666667f), image::Image::uv(0, 1, 4, 4, false), 0.000001f)
		<< "lower left corner of the image";
	EXPECT_VEC_NEAR(glm::vec2(0.0f, 0.25f), image::Image::uv(0, 1, 4, 4, true), 0.000001f)
		<< "upper left corner of the image";
}

TEST_F(ImageTest, testUVPixelConversionManual) {
	EXPECT_EQ(glm::ivec2(0, 1), image::Image::pixels(image::Image::uv(0, 1, 4, 4, false), 4, 4, Repeat, Repeat, false));
	EXPECT_EQ(glm::ivec2(0, 1), image::Image::pixels(image::Image::uv(0, 1, 4, 4, true), 4, 4, Repeat, Repeat, true));
}

TEST_F(ImageTest, testUVPixelConversion) {
	const image::ImagePtr &img = image::loadImage("test-palette-in.png");
	for (int x = 0; x < img->width(); ++x) {
		for (int y = 0; y < img->height(); ++y) {
			const glm::vec2 &uv = img->uv(x, y);
			const glm::ivec2 &pixels = img->pixels(uv);
			ASSERT_EQ(x, pixels.x) << "Failed to convert " << x << ":" << y << " to uv and back to pixels (uv: " << uv
								   << ", pixels: " << pixels << ") image: (w: " << img->width()
								   << ", h: " << img->height() << ")";
			ASSERT_EQ(y, pixels.y) << "Failed to convert " << x << ":" << y << " to uv and back to pixels (uv: " << uv
								   << ", pixels: " << pixels << ") image: (w: " << img->width()
								   << ", h: " << img->height() << ")";
		}
	}
}

TEST_F(ImageTest, testIsGrayScale) {
	image::ImagePtr img = image::createEmptyImage("gray");
	img->loadRGBA((const uint8_t *)img1, 6, 6);
	EXPECT_FALSE(img->isGrayScale());

	// Create a grayscale image manually
	image::ImagePtr grayImg = image::createEmptyImage("gray2");
	grayImg->loadRGBA((const uint8_t *)img1, 6, 6);
	for (int x = 0; x < 6; ++x) {
		for (int y = 0; y < 6; ++y) {
			grayImg->setColor(color::RGBA(128, 128, 128, 255), x, y);
		}
	}
	EXPECT_TRUE(grayImg->isGrayScale());
}

TEST_F(ImageTest, testSetColor) {
	image::ImagePtr img = image::createEmptyImage("setcolor");
	img->loadRGBA((const uint8_t *)img1, 6, 6);
	color::RGBA c(10, 20, 30, 40);
	EXPECT_TRUE(img->setColor(c, 0, 0));
	EXPECT_EQ(c, img->colorAt(0, 0));
	EXPECT_FALSE(img->setColor(c, -1, 0));
	EXPECT_FALSE(img->setColor(c, 6, 0));
	EXPECT_FALSE(img->setColor(c, 0, -1));
	EXPECT_FALSE(img->setColor(c, 0, 6));
}

TEST_F(ImageTest, testMakeOpaque) {
	image::ImagePtr img = image::createEmptyImage("opaque");
	img->loadRGBA((const uint8_t *)img1, 6, 6);
	// Set a pixel with alpha < 255
	img->setColor(color::RGBA(255, 0, 0, 128), 0, 0);
	EXPECT_EQ(128, img->colorAt(0, 0).a);
	img->makeOpaque();
	EXPECT_EQ(255, img->colorAt(0, 0).a);
}

TEST_F(ImageTest, testFlipVerticalRGBA) {
	uint8_t pixels[] = {
		255, 0, 0, 255,  0, 255, 0, 255,
		0, 0, 255, 255,  255, 255, 255, 255
	};
	// 2x2 image
	// R G
	// B W
	image::Image::flipVerticalRGBA(pixels, 2, 2);
	// Expected:
	// B W
	// R G
	EXPECT_EQ(0, pixels[0]); EXPECT_EQ(0, pixels[1]); EXPECT_EQ(255, pixels[2]); EXPECT_EQ(255, pixels[3]); // B
	EXPECT_EQ(255, pixels[4]); EXPECT_EQ(255, pixels[5]); EXPECT_EQ(255, pixels[6]); EXPECT_EQ(255, pixels[7]); // W
	EXPECT_EQ(255, pixels[8]); EXPECT_EQ(0, pixels[9]); EXPECT_EQ(0, pixels[10]); EXPECT_EQ(255, pixels[11]); // R
	EXPECT_EQ(0, pixels[12]); EXPECT_EQ(255, pixels[13]); EXPECT_EQ(0, pixels[14]); EXPECT_EQ(255, pixels[15]); // G
}

TEST_F(ImageTest, testResize) {
	image::ImagePtr img = image::createEmptyImage("resize");
	img->loadRGBA((const uint8_t *)img1, 6, 6);
	EXPECT_TRUE(img->resize(12, 12));
	EXPECT_EQ(12, img->width());
	EXPECT_EQ(12, img->height());
	// Check if content is preserved/scaled (basic check)
	// img1 has red at 0,0
	EXPECT_EQ(r, img->colorAt(0, 0));
}

TEST_F(ImageTest, testPngBase64) {
	image::ImagePtr img = image::createEmptyImage("base64");
	img->loadRGBA((const uint8_t *)img1, 6, 6);
	core::String base64 = img->pngBase64();
	EXPECT_FALSE(base64.empty());
	// Basic check for PNG header in base64 (iVBORw0KGgo)
	EXPECT_TRUE(base64.contains("iVBORw0KGgo"));
}

TEST_F(ImageTest, testLoadRGBA) {
	image::ImagePtr img = image::createEmptyImage("loadrgba");
	uint8_t data[] = {
		255, 0, 0, 255,
		0, 255, 0, 255
	};
	EXPECT_TRUE(img->loadRGBA(data, 2, 1));
	EXPECT_EQ(2, img->width());
	EXPECT_EQ(1, img->height());
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), img->colorAt(0, 0));
	EXPECT_EQ(color::RGBA(0, 255, 0, 255), img->colorAt(1, 0));
}

TEST_F(ImageTest, testLoadBGRA) {
	image::ImagePtr img = image::createEmptyImage("loadbgra");
	uint8_t data[] = {
		0, 0, 255, 255, // Red in BGRA (B G R A)
		0, 255, 0, 255  // Green in BGRA
	};
	io::MemoryReadStream stream(data, sizeof(data));
	EXPECT_TRUE(img->loadBGRA(stream, 2, 1));
	EXPECT_EQ(2, img->width());
	EXPECT_EQ(1, img->height());
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), img->colorAt(0, 0));
	EXPECT_EQ(color::RGBA(0, 255, 0, 255), img->colorAt(1, 0));
}

TEST_F(ImageTest, testPrint) {
	image::ImagePtr img = image::createEmptyImage("print");
	img->loadRGBA((const uint8_t *)img1, 6, 6);
	core::String output = image::print(img);
	EXPECT_FALSE(output.empty());
	EXPECT_TRUE(output.contains("w: 6, h: 6, d: 4"));
}

} // namespace image
