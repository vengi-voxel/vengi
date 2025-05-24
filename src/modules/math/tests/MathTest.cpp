/**
 * @file
 */

#include "math/Math.h"
#include "math/tests/TestMathHelper.h"
#include "app/tests/AbstractTest.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/euler_angles.hpp>

TEST(MathTest, testTransform) {
	const glm::mat4 &mat = glm::eulerAngleY(glm::radians(90.0f));
	const glm::ivec3 expected(0, 0, 1);
	const glm::ivec3 &destination = math::transform(mat, glm::ivec3(0), glm::vec3(0.0f));
	EXPECT_EQ(expected, destination) << expected << " vs " << destination;
}
