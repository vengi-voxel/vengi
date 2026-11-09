/**
 * @file
 */

#include "image/FontAtlas.h"
#include "app/tests/AbstractTest.h"
#include "image/Image.h"

namespace image {

class FontAtlasTest : public app::AbstractTest {};

TEST_F(FontAtlasTest, MetricsAndCoverageSupportTextLayout) {
	FontAtlas font;
	ASSERT_TRUE(font.init("font.ttf"));
	EXPECT_GT(font.width("Hello", 18), font.width("Hi", 18));
	EXPECT_NEAR(font.width("Hello", 36), 2 * font.width("Hello", 18), 0.001f);
	EXPECT_FLOAT_EQ(font.width("Hello\nHi", 18), font.width("Hello", 18));
	const auto &glyph = font.glyph('M');
	EXPECT_GT(glyph.bounds.z, 0);
	EXPECT_GT(glyph.uv.z, glyph.uv.x);
	int visible = 0;
	for (int y = 0; y < font.image()->height(); ++y) {
		for (int x = 0; x < font.image()->width(); ++x) {
			visible += font.image()->colorAt(x, y).a > 0;
		}
	}
	EXPECT_GT(visible, 1000);
	EXPECT_FALSE(font.init("font.ttf", -1));
}

TEST_F(FontAtlasTest, Utf8CountsCodepointsAndUsesConfiguredCoverage) {
	FontAtlas font;
	ASSERT_TRUE(font.init("font.ttf", 32, 1023));
	EXPECT_FLOAT_EQ(font.width("\xc3\xa4", 32), font.glyph(0xe4).advance);
	EXPECT_FLOAT_EQ(font.width("\xce\xa9", 32), font.glyph(0x3a9).advance);
	EXPECT_GT(font.glyph(0xe4).bounds.z, 0);
	EXPECT_FALSE(font.init("font.ttf", 32, 65535));
}

TEST_F(FontAtlasTest, RasterizeAndMeasureSupportVoxelFont) {
	FontAtlas font;
	ASSERT_TRUE(font.init("font.ttf"));
	uint8_t *bitmap = nullptr;
	int w = 0;
	int h = 0;
	int x0 = 0;
	int y0 = 0;
	ASSERT_TRUE(font.rasterize('A', 32, bitmap, w, h, x0, y0));
	EXPECT_NE(bitmap, nullptr);
	EXPECT_GT(w, 0);
	EXPECT_GT(h, 0);
	font.freeBitmap(bitmap);
	int mw = 0;
	int mh = 0;
	font.measure("Hi", 32, mw, mh);
	EXPECT_GT(mw, 0);
	EXPECT_GT(mh, 0);
	EXPECT_FALSE(font.rasterize('A', 0, bitmap, w, h, x0, y0));
}

} // namespace image
