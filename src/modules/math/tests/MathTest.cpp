/**
 * @file
 */

#include "math/Math.h"
#include "math/Functions.h"
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

TEST(MathTest, testTransformVec3) {
	const glm::mat4 &mat = glm::eulerAngleY(glm::radians(90.0f));
	const glm::vec3 expected(0.0f, 0.0f, 1.0f);
	const glm::vec3 &destination = math::transform(mat, glm::vec3(0.0f), glm::vec3(0.0f));
	EXPECT_NEAR(expected.x, destination.x, 0.0001f);
	EXPECT_NEAR(expected.y, destination.y, 0.0001f);
	EXPECT_NEAR(expected.z, destination.z, 0.0001f);
}

TEST(MathTest, testLogBase2) {
	EXPECT_EQ(0, math::logBase2(1));
	EXPECT_EQ(1, math::logBase2(2));
	EXPECT_EQ(2, math::logBase2(4));
	EXPECT_EQ(3, math::logBase2(8));
	EXPECT_EQ(4, math::logBase2(16));
	EXPECT_EQ(5, math::logBase2(32));
	EXPECT_EQ(10, math::logBase2(1024));
}

TEST(MathTest, testLogBase) {
	EXPECT_EQ(0, math::logBase(10, 1));
	EXPECT_EQ(1, math::logBase(10, 10));
	EXPECT_EQ(2, math::logBase(10, 100));
	EXPECT_EQ(3, math::logBase(10, 1000));
}
