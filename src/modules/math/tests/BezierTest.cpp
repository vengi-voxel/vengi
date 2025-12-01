/**
 * @file
 */

#include "math/Bezier.h"
#include "app/tests/AbstractTest.h"

namespace math {

class BezierTest : public app::AbstractTest {};

TEST_F(BezierTest, testGetPoint) {
	Bezier<float> b(glm::vec3(0.0f), glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(5.0f, 5.0f, 0.0f));
	glm::vec3 p = b.getPoint(0.0f);
	EXPECT_FLOAT_EQ(0.5f, p.x);
	EXPECT_FLOAT_EQ(0.5f, p.y);
	EXPECT_FLOAT_EQ(0.5f, p.z);

	p = b.getPoint(0.5f);
	EXPECT_FLOAT_EQ(5.5f, p.x);
	EXPECT_FLOAT_EQ(3.0f, p.y);
	EXPECT_FLOAT_EQ(0.5f, p.z);

	p = b.getPoint(1.0f);
	EXPECT_FLOAT_EQ(10.5f, p.x);
	EXPECT_FLOAT_EQ(0.5f, p.y);
	EXPECT_FLOAT_EQ(0.5f, p.z);
}

} // namespace math
