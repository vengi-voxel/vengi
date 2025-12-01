/**
 * @file
 */

#include "math/Ray.h"
#include "app/tests/AbstractTest.h"

namespace math {

class RayTest : public app::AbstractTest {};

TEST_F(RayTest, testIsValid) {
	Ray r(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	EXPECT_TRUE(r.isValid());
}

TEST_F(RayTest, testIsInvalidOrigin) {
	Ray r(glm::vec3(NAN), glm::vec3(0.0f, 1.0f, 0.0f));
	EXPECT_FALSE(r.isValid());
}

TEST_F(RayTest, testIsInvalidDirection) {
	Ray r(glm::vec3(0.0f), glm::vec3(NAN));
	EXPECT_FALSE(r.isValid());
}

} // namespace math
