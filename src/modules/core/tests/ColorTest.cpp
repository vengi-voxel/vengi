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
}

}
