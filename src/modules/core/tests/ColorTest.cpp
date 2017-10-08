/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Color.h"
#include <SDL_endian.h>

namespace core {

TEST(ColorTest, testRGBA) {
	core::RGBA color;
	color.rgba = SDL_SwapLE32(0xff6699fe);
	EXPECT_EQ(0xfe, color.r);
	EXPECT_EQ(0x99, color.g);
	EXPECT_EQ(0x66, color.b);
	EXPECT_EQ(0xff, color.a);

	const glm::vec4 fcolor = core::Color::fromRGBA(color.rgba);
	EXPECT_FLOAT_EQ(color.r / core::Color::magnitudef, fcolor.r);
	EXPECT_FLOAT_EQ(color.g / core::Color::magnitudef, fcolor.g);
	EXPECT_FLOAT_EQ(color.b / core::Color::magnitudef, fcolor.b);
	EXPECT_FLOAT_EQ(color.a / core::Color::magnitudef, fcolor.a);
	EXPECT_FLOAT_EQ(1.0f, fcolor.a);

	core::RGBA convertedBack;
	convertedBack.rgba = core::Color::getRGBA(fcolor);
	EXPECT_EQ(0xfe, convertedBack.r);
	EXPECT_EQ(0x99, convertedBack.g);
	EXPECT_EQ(0x66, convertedBack.b);
	EXPECT_EQ(0xff, convertedBack.a);
}

}
