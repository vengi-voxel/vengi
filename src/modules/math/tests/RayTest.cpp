/**
 * @file
 */

#include "math/Ray.h"
#include "app/tests/AbstractTest.h"
#include "math/Vec3f.h"

namespace math {

class RayTest : public app::AbstractTest {};

TEST_F(RayTest, testIsValid) {
	Ray r(math::Vec3f(0.0f), math::Vec3f(0.0f, 1.0f, 0.0f));
	EXPECT_TRUE(r.isValid());
}

TEST_F(RayTest, testIsInvalidOrigin) {
	Ray r(math::Vec3f(math::NotANumber), math::Vec3f(0.0f, 1.0f, 0.0f));
	EXPECT_FALSE(r.isValid());
}

TEST_F(RayTest, testIsInvalidDirection) {
	Ray r(math::Vec3f(0.0f), math::Vec3f(math::NotANumber));
	EXPECT_FALSE(r.isValid());
}

} // namespace math
