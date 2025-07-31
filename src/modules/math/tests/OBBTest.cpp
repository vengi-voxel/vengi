/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "math/OBB.h"
#include "TestMathHelper.h"

namespace math {

class OBBTest : public app::AbstractTest {
};

TEST_F(OBBTest, testContains) {
	OBBF obb(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f));
	ASSERT_TRUE(obb.contains(glm::vec3(0, 0, 0)));
	ASSERT_FALSE(obb.contains(glm::vec3(1, 5, 1)));
}

TEST_F(OBBTest, testIntersects) {
	OBBF obb(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f));
	float distance;
	ASSERT_TRUE(obb.intersect(glm::vec3(10.0f, 0.0, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), distance));
	ASSERT_FLOAT_EQ(distance, 9.0f);
}

}
