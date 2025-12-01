/**
 * @file
 */

#include "color/CMYK.h"
#include "app/tests/AbstractTest.h"
#include <gtest/gtest.h>

namespace color {

class CMYKTest : public app::AbstractTest {
};

TEST_F(CMYKTest, testToRGB) {
	CMYK cmyk(0.0f, 0.0f, 0.0f, 1.0f); // Black
	RGBA rgb = cmyk.toRGB();
	EXPECT_EQ(0, rgb.r);
	EXPECT_EQ(0, rgb.g);
	EXPECT_EQ(0, rgb.b);

	cmyk = CMYK(0.0f, 0.0f, 0.0f, 0.0f); // White
	rgb = cmyk.toRGB();
	EXPECT_EQ(255, rgb.r);
	EXPECT_EQ(255, rgb.g);
	EXPECT_EQ(255, rgb.b);

	cmyk = CMYK(0.0f, 1.0f, 1.0f, 0.0f); // Red
	rgb = cmyk.toRGB();
	EXPECT_EQ(255, rgb.r);
	EXPECT_EQ(0, rgb.g);
	EXPECT_EQ(0, rgb.b);
}

TEST_F(CMYKTest, testFromRGB) {
	RGBA rgb(0, 0, 0); // Black
	CMYK cmyk = CMYK::fromRGB(rgb);
	EXPECT_FLOAT_EQ(1.0f, cmyk.cmyk[3]); // K

	rgb = RGBA(255, 255, 255); // White
	cmyk = CMYK::fromRGB(rgb);
	EXPECT_FLOAT_EQ(0.0f, cmyk.cmyk[0]);
	EXPECT_FLOAT_EQ(0.0f, cmyk.cmyk[1]);
	EXPECT_FLOAT_EQ(0.0f, cmyk.cmyk[2]);
	EXPECT_FLOAT_EQ(0.0f, cmyk.cmyk[3]);

	rgb = RGBA(255, 0, 0); // Red
	cmyk = CMYK::fromRGB(rgb);
	EXPECT_FLOAT_EQ(0.0f, cmyk.cmyk[0]); // C
	EXPECT_FLOAT_EQ(1.0f, cmyk.cmyk[1]); // M
	EXPECT_FLOAT_EQ(1.0f, cmyk.cmyk[2]); // Y
	EXPECT_FLOAT_EQ(0.0f, cmyk.cmyk[3]); // K
}

TEST_F(CMYKTest, testAssignment) {
	CMYK cmyk1(0.1f, 0.2f, 0.3f, 0.4f);
	CMYK cmyk2(0.0f, 0.0f, 0.0f, 0.0f);
	cmyk2 = cmyk1;
	EXPECT_FLOAT_EQ(0.1f, cmyk2.cmyk[0]);
	EXPECT_FLOAT_EQ(0.2f, cmyk2.cmyk[1]);
	EXPECT_FLOAT_EQ(0.3f, cmyk2.cmyk[2]);
	EXPECT_FLOAT_EQ(0.4f, cmyk2.cmyk[3]);
}

} // namespace color
