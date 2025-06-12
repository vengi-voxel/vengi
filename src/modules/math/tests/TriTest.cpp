/**
 * @file
 */

#include "math/Tri.h"
#include "app/tests/AbstractTest.h"

namespace math {

class TriTest : public app::AbstractTest {};

TEST_F(TriTest, testMinsMaxs) {
	Tri tri;
	tri.setVertices(glm::vec3(-20, -10, -23), glm::vec3(-10, -30, 23), glm::vec3(20, 30, 40));
	const glm::vec3 mins = tri.mins();
	const glm::vec3 maxs = tri.maxs();
	EXPECT_FLOAT_EQ(-20.0f, mins.x);
	EXPECT_FLOAT_EQ(-30.0f, mins.y);
	EXPECT_FLOAT_EQ(-23.0f, mins.z);
	EXPECT_FLOAT_EQ(20.0f, maxs.x);
	EXPECT_FLOAT_EQ(30.0f, maxs.y);
	EXPECT_FLOAT_EQ(40.0f, maxs.z);
}

TEST_F(TriTest, testFlat) {
	Tri tri;
	tri.setVertices(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1));
	EXPECT_TRUE(tri.flat()) << tri.normal().x << ":" << tri.normal().y << ":" << tri.normal().z;
	tri.setVertices(glm::vec3(0, 0, 0), glm::vec3(1, 1, 0), glm::vec3(0, 0, 1));
	EXPECT_FALSE(tri.flat()) << tri.normal().x << ":" << tri.normal().y << ":" << tri.normal().z;
}

} // namespace math
