/**
 * @file
 */

#include <gtest/gtest.h>
#include "color/Color.h"
#include "color/RGBA.h"
#include "core/collection/BufferView.h"
#include "core/Endian.h"

namespace color {

TEST(ColorTest, testRGBA) {
	color::RGBA color;
	color.rgba = core_swap32le(0xff6699fe);
	EXPECT_EQ(0xfe, color.r);
	EXPECT_EQ(0x99, color.g);
	EXPECT_EQ(0x66, color.b);
	EXPECT_EQ(0xff, color.a);

	const glm::vec4 fcolor = color::Color::fromRGBA(color);
	EXPECT_FLOAT_EQ(color.r / color::Color::magnitudef, fcolor.r);
	EXPECT_FLOAT_EQ(color.g / color::Color::magnitudef, fcolor.g);
	EXPECT_FLOAT_EQ(color.b / color::Color::magnitudef, fcolor.b);
	EXPECT_FLOAT_EQ(color.a / color::Color::magnitudef, fcolor.a);
	EXPECT_FLOAT_EQ(1.0f, fcolor.a);

	color::RGBA convertedBack = color::Color::getRGBA(fcolor);
	EXPECT_EQ(0xfe, convertedBack.r);
	EXPECT_EQ(0x99, convertedBack.g);
	EXPECT_EQ(0x66, convertedBack.b);
	EXPECT_EQ(0xff, convertedBack.a);
}

TEST(ColorTest, testHex) {
	EXPECT_EQ(color::RGBA(255, 255, 255, 255), color::Color::fromHex("#ffffff"));
	EXPECT_EQ(color::RGBA(255, 255, 255, 255), color::Color::fromHex("0xffffff"));
	EXPECT_EQ(color::RGBA(255, 255, 255, 255), color::Color::fromHex("0xffffffff"));
	EXPECT_EQ(color::RGBA(0), color::Color::fromHex("0x00000000"));
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), color::Color::fromHex("0xff0000ff"));
	EXPECT_EQ(color::RGBA(255, 0, 0, 255), color::Color::fromHex("#ff0000ff"));
}

TEST(ColorTest, testDistanceMin) {
	const color::RGBA color1(255, 0, 0, 255);
	const color::RGBA color2(255, 0, 0, 255);
	EXPECT_FLOAT_EQ(0.0f, color::Color::getDistance(color1, color2, color::Color::Distance::HSB));
	EXPECT_FLOAT_EQ(0.0f, color::Color::getDistance(color1, color2, color::Color::Distance::Approximation));
}

TEST(ColorTest, testDistanceMax) {
	const color::RGBA color1(0, 0, 0, 255);
	const color::RGBA color2(255, 255, 255, 255);
	EXPECT_FLOAT_EQ(0.1f, color::Color::getDistance(color1, color2, color::Color::Distance::HSB));
	EXPECT_FLOAT_EQ(584970.0f, color::Color::getDistance(color1, color2, color::Color::Distance::Approximation));
}
} // namespace color
