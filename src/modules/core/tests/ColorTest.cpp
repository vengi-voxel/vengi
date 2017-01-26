/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Color.h"

namespace core {

TEST(ColorTest, testRGBA) {
	core::RGBA color;
	color.rgba = 0xff6699ff;
	EXPECT_EQ(0xff, color.r);
	EXPECT_EQ(0x99, color.g);
	EXPECT_EQ(0x66, color.b);
	EXPECT_EQ(0xff, color.a);

	const glm::vec4 fcolor = core::Color::fromRGBA(color.rgba);
	EXPECT_FLOAT_EQ(1.0f, fcolor.a);
	EXPECT_FLOAT_EQ(color.b / (float)core::Color::magnitude, fcolor.b);
	EXPECT_FLOAT_EQ(color.g / (float)core::Color::magnitude, fcolor.g);
	EXPECT_FLOAT_EQ(1.0f, fcolor.r);
}

}
