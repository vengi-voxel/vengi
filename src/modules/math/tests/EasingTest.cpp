/**
 * @file
 */

#include "math/Easing.h"
#include "app/tests/AbstractTest.h"

namespace util {
namespace easing {

class EasingTest : public app::AbstractTest {};

TEST_F(EasingTest, testLinear) {
	EXPECT_DOUBLE_EQ(0.0, linear(0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(0.5, linear(5.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(1.0, linear(10.0, 0.0, 10.0));
}

TEST_F(EasingTest, testFull) {
	EXPECT_DOUBLE_EQ(0.0, full(0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(1.0, full(5.0, 0.0, 10.0)); // round(0.5) -> 1.0
	EXPECT_DOUBLE_EQ(1.0, full(10.0, 0.0, 10.0));
}

TEST_F(EasingTest, testQuadInOut) {
	EXPECT_DOUBLE_EQ(0.0, quadInOut(0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(0.5, quadInOut(5.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(1.0, quadInOut(10.0, 0.0, 10.0));
}

TEST_F(EasingTest, testQuadOut) {
	EXPECT_DOUBLE_EQ(0.0, quadOut(0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(0.75, quadOut(5.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(1.0, quadOut(10.0, 0.0, 10.0));
}

TEST_F(EasingTest, testQuadIn) {
	EXPECT_DOUBLE_EQ(0.0, quadIn(0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(0.25, quadIn(5.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(1.0, quadIn(10.0, 0.0, 10.0));
}

TEST_F(EasingTest, testCubicIn) {
	EXPECT_DOUBLE_EQ(0.0, cubicIn(0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(0.125, cubicIn(5.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(1.0, cubicIn(10.0, 0.0, 10.0));
}

TEST_F(EasingTest, testCubicOut) {
	EXPECT_DOUBLE_EQ(0.0, cubicOut(0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(0.875, cubicOut(5.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(1.0, cubicOut(10.0, 0.0, 10.0));
}

TEST_F(EasingTest, testCubicInOut) {
	EXPECT_DOUBLE_EQ(0.0, cubicInOut(0.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(0.5, cubicInOut(5.0, 0.0, 10.0));
	EXPECT_DOUBLE_EQ(1.0, cubicInOut(10.0, 0.0, 10.0));
}

} // namespace easing
} // namespace util
