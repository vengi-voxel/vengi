/**
 * @file
 */

#include "math/Axis.h"
#include "app/tests/AbstractTest.h"

namespace math {

class AxisTest : public app::AbstractTest {};

TEST_F(AxisTest, testGetIndexForAxis) {
	EXPECT_EQ(0, getIndexForAxis(Axis::X));
	EXPECT_EQ(1, getIndexForAxis(Axis::Y));
	EXPECT_EQ(2, getIndexForAxis(Axis::Z));
}

TEST_F(AxisTest, testGetCharForAxis) {
	EXPECT_STREQ("x", getCharForAxis(Axis::X));
	EXPECT_STREQ("y", getCharForAxis(Axis::Y));
	EXPECT_STREQ("z", getCharForAxis(Axis::Z));
	EXPECT_STREQ("none", getCharForAxis(Axis::None));
}

TEST_F(AxisTest, testToAxis) {
	EXPECT_EQ(Axis::X, toAxis("x"));
	EXPECT_EQ(Axis::Y, toAxis("y"));
	EXPECT_EQ(Axis::Z, toAxis("z"));
	EXPECT_EQ(Axis::X, toAxis("X"));
	EXPECT_EQ(Axis::Y, toAxis("Y"));
	EXPECT_EQ(Axis::Z, toAxis("Z"));
	EXPECT_EQ(Axis::None, toAxis("none"));
	EXPECT_EQ(Axis::None, toAxis("invalid"));
}

} // namespace math
