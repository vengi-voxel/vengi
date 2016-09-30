/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/Plane.h"

namespace core {

class PlaneTest : public core::AbstractTest {
};

TEST_F(PlaneTest, testOrigin) {
	const Plane p(glm::up, glm::vec3(0.0f));
	ASSERT_TRUE(p.isFrontSide(glm::up)) << p.distanceToPlane(glm::up);
	ASSERT_TRUE(p.isBackSide(glm::down)) << p.distanceToPlane(glm::down);
}

TEST_F(PlaneTest, testWithDistance) {
	const float y = 10.0;
	const Plane p(glm::up, glm::vec3(0.0, y, 0.0f));
	const glm::vec3 pos1 = glm::up * y + 0.01f;
	const glm::vec3 pos2 = glm::up * y - 0.01f;
	ASSERT_TRUE(p.isFrontSide(pos1)) << p.distanceToPlane(pos1);
	ASSERT_TRUE(p.isBackSide(pos2)) << p.distanceToPlane(pos2);
}

}
