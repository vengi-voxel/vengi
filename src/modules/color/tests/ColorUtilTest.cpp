/**
 * @file
 */

#include "color/Color.h"
#include "app/tests/AbstractTest.h"
#include "color/ColorUtil.h"
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
} // namespace color
