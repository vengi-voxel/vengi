/**
 * @file
 */

#include "color/RGBA.h"
#include "app/tests/AbstractTest.h"
#include <gtest/gtest.h>

namespace color {

class RGBATest : public app::AbstractTest {};

TEST_F(RGBATest, testConstructors) {
	RGBA c1;
	EXPECT_EQ(0u, c1.rgba);
	EXPECT_EQ(0, c1.r);
	EXPECT_EQ(0, c1.g);
	EXPECT_EQ(0, c1.b);
	EXPECT_EQ(0, c1.a);

	RGBA c2(255, 128, 64, 32);
	EXPECT_EQ(255, c2.r);
	EXPECT_EQ(128, c2.g);
	EXPECT_EQ(64, c2.b);
	EXPECT_EQ(32, c2.a);
}

TEST_F(RGBATest, testOperators) {
	RGBA c1(10, 20, 30, 40);
	RGBA c2(10, 20, 30, 40);
	RGBA c3(11, 20, 30, 40);

	EXPECT_TRUE(c1 == c2);
	EXPECT_FALSE(c1 == c3);
	EXPECT_FALSE(c1 != c2);
	EXPECT_TRUE(c1 != c3);

	c1 = c3;
	EXPECT_TRUE(c1 == c3);

	c1 = 0x12345678u;
	EXPECT_EQ(0x12345678u, c1.rgba);
}

TEST_F(RGBATest, testIndexOperator) {
	RGBA c(10, 20, 30, 40);
	EXPECT_EQ(10, c[0]);
	EXPECT_EQ(20, c[1]);
	EXPECT_EQ(30, c[2]);
	EXPECT_EQ(40, c[3]);
	EXPECT_EQ(0, c[4]);
}

TEST_F(RGBATest, testMix) {
	RGBA c1(0, 0, 0, 0);
	RGBA c2(100, 100, 100, 100);
	RGBA mixed = RGBA::mix(c1, c2, 0.5f);
	EXPECT_EQ(50, mixed.r);
	EXPECT_EQ(50, mixed.g);
	EXPECT_EQ(50, mixed.b);
	EXPECT_EQ(100, mixed.a);

	mixed = RGBA::mix(c1, c2, 0.0f);
	EXPECT_EQ(0, mixed.r);

	mixed = RGBA::mix(c1, c2, 1.0f);
	EXPECT_EQ(100, mixed.r);
}

TEST_F(RGBATest, testBrightness) {
	RGBA white(255, 255, 255);
	EXPECT_NEAR(255.0, white.brightness(), 0.001);

	RGBA black(0, 0, 0);
	EXPECT_NEAR(0.0, black.brightness(), 0.001);

	RGBA red(255, 0, 0);
	// 0.299 * 255
	EXPECT_NEAR(76.245, red.brightness(), 0.001);
}

} // namespace color
