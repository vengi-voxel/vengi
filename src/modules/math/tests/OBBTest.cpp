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
	OBB<float> obb(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f));
	ASSERT_TRUE(obb.contains(glm::vec3(0, 0, 0)));
	ASSERT_FALSE(obb.contains(glm::vec3(1, 5, 1)));
}

TEST_F(OBBTest, testBounds) {
	OBB<float> obb(glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3x3(1.0f));
	core::Pair<glm::vec3, glm::vec3> bounds = obb.bounds();
	ASSERT_VEC3_NEAR(bounds.first, glm::vec3(-1.0f), 0.000001f);
	ASSERT_VEC3_NEAR(bounds.second, glm::vec3(1.0f), 0.000001f);
}

}
