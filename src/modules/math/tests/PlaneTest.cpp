/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "math/Plane.h"
#include "core/GLM.h"

namespace math {

class PlaneTest : public core::AbstractTest {
};

TEST_F(PlaneTest, testOrigin) {
	const Plane p(glm::up, glm::vec3(0.0f));
	EXPECT_TRUE(p.isFrontSide(glm::up)) << p.distanceToPlane(glm::up);
	EXPECT_TRUE(p.isBackSide(glm::down)) << p.distanceToPlane(glm::down);
}

TEST_F(PlaneTest, testWithNormalUpwards) {
	const float y = 10.0;
	const Plane p(glm::up, glm::vec3(0.0f, y, 0.0f));
	EXPECT_TRUE(p.isBackSide(glm::up));
	EXPECT_TRUE(p.isBackSide(glm::vec3(0.0f, y - 0.1f, 0.0f)));
	EXPECT_TRUE(p.isFrontSide(glm::vec3(0.0f, y + 0.1f, 0.0f)));
}

TEST_F(PlaneTest, testWithNormalLeft) {
	const float x = 10.0;
	const Plane p(glm::left, glm::vec3(x, 0.0f, 0.0f));
	EXPECT_TRUE(p.isFrontSide(glm::zero<glm::vec3>()));
	EXPECT_TRUE(p.isBackSide(glm::vec3(x + 0.1f, 0.0f, 0.0f)));
	EXPECT_TRUE(p.isFrontSide(glm::vec3(x - 0.1f, 0.0f, 0.0f)));
}

}
