/**
 * @file
 */

#include "color/ColorUtil.h"
#include "app/tests/AbstractTest.h"
#include "color/Color.h"
#include "color/RGBA.h"
#include "core/Endian.h"
#include "core/collection/BufferView.h"
#include <gtest/gtest.h>

namespace color {

class ColorUtilTest : public app::AbstractTest {
public:
	ColorUtilTest() : app::AbstractTest() {
	}
};

TEST_F(ColorUtilTest, testRGBA) {
	color::RGBA color;
	color.rgba = core_swap32le(0xff6699fe);
	EXPECT_EQ(0xfe, color.r);
	EXPECT_EQ(0x99, color.g);
	EXPECT_EQ(0x66, color.b);
	EXPECT_EQ(0xff, color.a);

	const glm::vec4 fcolor = color::fromRGBA(color);
	EXPECT_FLOAT_EQ(color.r / color::magnitudef, fcolor.r);
	EXPECT_FLOAT_EQ(color.g / color::magnitudef, fcolor.g);
	EXPECT_FLOAT_EQ(color.b / color::magnitudef, fcolor.b);
	EXPECT_FLOAT_EQ(color.a / color::magnitudef, fcolor.a);
	EXPECT_FLOAT_EQ(1.0f, fcolor.a);

	color::RGBA convertedBack = color::toRGBA(fcolor);
	EXPECT_EQ(0xfe, convertedBack.r);
	EXPECT_EQ(0x99, convertedBack.g);
	EXPECT_EQ(0x66, convertedBack.b);
	EXPECT_EQ(0xff, convertedBack.a);
}

TEST_F(ColorUtilTest, testHex) {
	EXPECT_EQ(color::RGBA(255, 255, 255, 255), color::fromHex("#ffffff"));
	EXPECT_EQ(color::RGBA(255, 255, 255, 255), color::fromHex("0xffffff"));
	EXPECT_EQ(color::RGBA(255, 255, 255, 255), color::fromHex("0xffffffff"));
	EXPECT_EQ(color::RGBA(0), color::fromHex("0x00000000"));
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), color::fromHex("0xff0000ff"));
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), color::fromHex("#ff0000ff"));
}

TEST_F(ColorUtilTest, testToHex) {
	EXPECT_EQ("#ffffffff", color::toHex(color::RGBA(255, 255, 255, 255), true));
	EXPECT_EQ("ffffffff", color::toHex(color::RGBA(255, 255, 255, 255), false));
	EXPECT_EQ("#ff0000ff", color::toHex(color::RGBA(255, 0, 0, 255), true));
	EXPECT_EQ("#00ff00ff", color::toHex(color::RGBA(0, 255, 0, 255), true));
	EXPECT_EQ("#0000ffff", color::toHex(color::RGBA(0, 0, 255, 255), true));
}

TEST_F(ColorUtilTest, testDistanceMin) {
	const color::RGBA color1(255, 0, 0, 255);
	const color::RGBA color2(255, 0, 0, 255);
	EXPECT_FLOAT_EQ(0.0f, color::getDistance(color1, color2, color::Distance::HSB));
	EXPECT_FLOAT_EQ(0.0f, color::getDistance(color1, color2, color::Distance::Approximation));
}

TEST_F(ColorUtilTest, testDistanceMax) {
	const color::RGBA color1(0, 0, 0, 255);
	const color::RGBA color2(255, 255, 255, 255);
	EXPECT_FLOAT_EQ(0.1f, color::getDistance(color1, color2, color::Distance::HSB));
	EXPECT_FLOAT_EQ(584970.0f, color::getDistance(color1, color2, color::Distance::Approximation));
}

TEST_F(ColorUtilTest, testHSB) {
	float h, s, b;
	color::getHSB(color::RGBA(255, 0, 0), h, s, b);
	EXPECT_FLOAT_EQ(0.0f, h);
	EXPECT_FLOAT_EQ(1.0f, s);
	EXPECT_FLOAT_EQ(1.0f, b);

	color::getHSB(color::RGBA(0, 255, 0), h, s, b);
	EXPECT_NEAR(0.333333f, h, 0.0001f);
	EXPECT_FLOAT_EQ(1.0f, s);
	EXPECT_FLOAT_EQ(1.0f, b);

	color::getHSB(color::RGBA(0, 0, 255), h, s, b);
	EXPECT_NEAR(0.666666f, h, 0.0001f);
	EXPECT_FLOAT_EQ(1.0f, s);
	EXPECT_FLOAT_EQ(1.0f, b);

	color::RGBA rgba = color::fromHSB(0.0f, 1.0f, 1.0f);
	EXPECT_EQ(255, rgba.r);
	EXPECT_EQ(0, rgba.g);
	EXPECT_EQ(0, rgba.b);
	EXPECT_EQ(255, rgba.a);
}

TEST_F(ColorUtilTest, testCIELab) {
	float l, a, b;
	color::getCIELab(color::RGBA(255, 0, 0), l, a, b);
	EXPECT_NEAR(53.2328f, l, 0.01f);
	EXPECT_NEAR(80.1093f, a, 0.01f);
	EXPECT_NEAR(67.2200f, b, 0.01f);

	color::RGBA rgba = color::fromCIELab(glm::vec4(l, a, b, 1.0f));
	EXPECT_NEAR(255, (int)rgba.r, 1);
	EXPECT_NEAR(0, (int)rgba.g, 1);
	EXPECT_NEAR(0, (int)rgba.b, 1);
	EXPECT_EQ(255, rgba.a);
}

TEST_F(ColorUtilTest, testDeltaE76) {
	EXPECT_NEAR(0.0, color::deltaE76(color::RGBA(255, 0, 0), color::RGBA(255, 0, 0)), 0.001);
	EXPECT_NEAR(0.0, color::deltaE76(color::RGBA(0, 255, 0), color::RGBA(0, 255, 0)), 0.001);
	EXPECT_NEAR(0.0, color::deltaE76(color::RGBA(0, 0, 255), color::RGBA(0, 0, 255)), 0.001);
	EXPECT_GT(color::deltaE76(color::RGBA(255, 0, 0), color::RGBA(0, 255, 0)), 0.0);
}

TEST_F(ColorUtilTest, testGray) {
	glm::vec3 g = color::gray(glm::vec3(1.0f, 0.0f, 0.0f));
	EXPECT_NEAR(0.21f, g.r, 0.001f);
	EXPECT_NEAR(0.21f, g.g, 0.001f);
	EXPECT_NEAR(0.21f, g.b, 0.001f);
}

TEST_F(ColorUtilTest, testBrightness) {
	EXPECT_EQ(255, color::brightness(color::RGBA(255, 255, 255)));
	EXPECT_EQ(0, color::brightness(color::RGBA(0, 0, 0)));
	EXPECT_EQ(255, color::brightness(color::RGBA(255, 0, 0)));
}

TEST_F(ColorUtilTest, testDarkerBrighter) {
	color::RGBA red(255, 0, 0);
	color::RGBA darkRed = color::darker(red, 0.5f);
	EXPECT_LT(darkRed.r, red.r);
	EXPECT_EQ(darkRed.g, red.g);
	EXPECT_EQ(darkRed.b, red.b);

	color::RGBA brightRed = color::brighter(darkRed, 2.0f); // approximate inverse
	EXPECT_GT(brightRed.r, darkRed.r);
}

TEST_F(ColorUtilTest, testFlattenRGB) {
	color::RGBA c(100, 150, 200, 255);
	color::RGBA flattened = color::flattenRGB(c.r, c.g, c.b, c.a, 10);
	EXPECT_EQ(100, flattened.r);
	EXPECT_EQ(150, flattened.g);
	EXPECT_EQ(200, flattened.b);

	flattened = color::flattenRGB(c.r, c.g, c.b, c.a, 255);
	EXPECT_EQ(0, flattened.r);
	EXPECT_EQ(0, flattened.g);
	EXPECT_EQ(0, flattened.b);
}

TEST_F(ColorUtilTest, testContrastTextColor) {
	glm::vec4 white(1.0f);
	glm::vec4 black(0.0f, 0.0f, 0.0f, 1.0f);
	EXPECT_EQ(black, color::contrastTextColor(white));
	EXPECT_EQ(white, color::contrastTextColor(black));
}

TEST_F(ColorUtilTest, testPrint) {
	color::RGBA red(255, 0, 0, 255);
	core::String output = color::print(red, true);
	EXPECT_TRUE(output.contains("#ff0000ff"));
	EXPECT_TRUE(output.contains("\033[38;2;255;0;0m"));
	EXPECT_TRUE(output.contains("\033[48;2;255;0;0m"));

	output = color::print(red, false);
	EXPECT_FALSE(output.contains("#ff0000ff"));
	EXPECT_TRUE(output.contains("\033[38;2;255;0;0m"));
}

TEST_F(ColorUtilTest, testSrgbToLinear) {
	EXPECT_NEAR(0.0, color::srgbToLinear(0), 0.0001);
	EXPECT_NEAR(1.0, color::srgbToLinear(255), 0.0001);
	// 127/255 = ~0.498. Linear approx ~0.2122
	EXPECT_NEAR(0.2122, color::srgbToLinear(127), 0.001);
}

TEST_F(ColorUtilTest, testRgbToXyz) {
	double X, Y, Z;
	color::rgbToXyz(255, 0, 0, X, Y, Z);
	// Red in sRGB to XYZ (D65)
	// X ~ 0.4124, Y ~ 0.2126, Z ~ 0.0193
	EXPECT_NEAR(0.4124, X, 0.001);
	EXPECT_NEAR(0.2126, Y, 0.001);
	EXPECT_NEAR(0.0193, Z, 0.001);
}

TEST_F(ColorUtilTest, testXyzToLab) {
	double L, a, b;
	// D65 White point XYZ -> Lab (100, 0, 0)
	// X=0.95047, Y=1.00000, Z=1.08883
	color::xyzToLab(0.95047, 1.00000, 1.08883, L, a, b);
	EXPECT_NEAR(100.0, L, 0.01);
	EXPECT_NEAR(0.0, a, 0.01);
	EXPECT_NEAR(0.0, b, 0.01);
}

TEST_F(ColorUtilTest, testGetDistanceHSBValues) {
	color::RGBA red(255, 0, 0, 255);
	// Distance to itself should be 0
	EXPECT_FLOAT_EQ(0.0f, color::getDistance(red, 0.0f, 1.0f, 1.0f));
	// Distance to Green (Hue 0.333)
	EXPECT_GT(color::getDistance(red, 0.333f, 1.0f, 1.0f), 0.0f);
}

TEST_F(ColorUtilTest, testGetRGBAFromVec3) {
	glm::vec3 v(1.0f, 0.0f, 0.0f);
	color::RGBA c = color::getRGBA(v);
	EXPECT_EQ(255, c.r);
	EXPECT_EQ(0, c.g);
	EXPECT_EQ(0, c.b);
	EXPECT_EQ(255, c.a);
}

TEST_F(ColorUtilTest, testGetHSBVec4) {
	glm::vec4 v(1.0f, 0.0f, 0.0f, 1.0f);
	float h, s, b;
	color::getHSB(v, h, s, b);
	EXPECT_FLOAT_EQ(0.0f, h);
	EXPECT_FLOAT_EQ(1.0f, s);
	EXPECT_FLOAT_EQ(1.0f, b);
}

TEST_F(ColorUtilTest, testGetCIELabVec4) {
	glm::vec4 v(1.0f, 0.0f, 0.0f, 1.0f);
	float l, a, b;
	color::getCIELab(v, l, a, b);
	EXPECT_NEAR(53.2328f, l, 0.01f);
	EXPECT_NEAR(80.1093f, a, 0.01f);
	EXPECT_NEAR(67.2200f, b, 0.01f);
}

TEST_F(ColorUtilTest, testAlpha) {
	glm::vec4 v(1.0f, 1.0f, 1.0f, 0.5f);
	glm::vec4 v2 = color::alpha(v, 0.8f);
	EXPECT_FLOAT_EQ(0.8f, v2.a);
	EXPECT_FLOAT_EQ(1.0f, v2.r);

	color::RGBA c(255, 255, 255, 128);
	color::RGBA c2 = color::alpha(c, 200);
	EXPECT_EQ(200, c2.a);
	EXPECT_EQ(255, c2.r);
}

TEST_F(ColorUtilTest, testBrightnessVec4) {
	glm::vec4 v(1.0f, 0.5f, 0.0f, 1.0f);
	// Brightness is max(r, g, b)
	EXPECT_FLOAT_EQ(1.0f, color::brightness(v));

	glm::vec4 v2(0.1f, 0.5f, 0.2f, 1.0f);
	EXPECT_FLOAT_EQ(0.5f, color::brightness(v2));
}

TEST_F(ColorUtilTest, testIntensity) {
	glm::vec4 v(1.0f, 0.5f, 0.0f, 1.0f);
	// Intensity is average(r, g, b)
	EXPECT_FLOAT_EQ(0.5f, color::intensity(v));
}

TEST_F(ColorUtilTest, testGrayVec4) {
	glm::vec4 v(1.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 g = color::gray(v);
	EXPECT_NEAR(0.21f, g.r, 0.01f);
	EXPECT_NEAR(0.21f, g.g, 0.01f);
	EXPECT_NEAR(0.21f, g.b, 0.01f);
	EXPECT_FLOAT_EQ(1.0f, g.a);
}

TEST_F(ColorUtilTest, testDarkerBrighterVec4) {
	glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 darkRed = color::darker(red, 0.5f);
	EXPECT_LT(darkRed.r, red.r);
	EXPECT_FLOAT_EQ(darkRed.g, red.g);
	EXPECT_FLOAT_EQ(darkRed.b, red.b);

	glm::vec4 brightRed = color::brighter(darkRed, 2.0f);
	EXPECT_GT(brightRed.r, darkRed.r);
}

} // namespace color
