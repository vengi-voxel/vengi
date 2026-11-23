/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "math/Plane.h"
#include "math/Vec3f.h"
#include "math/Axes.h"

namespace math {

class PlaneTest : public app::AbstractTest {
};

TEST_F(PlaneTest, testOrigin) {
	const Plane p(math::up(), math::Vec3f(0.0f));
	EXPECT_TRUE(p.isFrontSide(math::up())) << p.distanceToPlane(math::up());
	EXPECT_TRUE(p.isBackSide(math::down())) << p.distanceToPlane(math::down());
}

TEST_F(PlaneTest, testWithNormalUpwards) {
	const float y = 10.0;
	const Plane p(math::up(), math::Vec3f(0.0f, y, 0.0f));
	EXPECT_TRUE(p.isBackSide(math::up()));
	EXPECT_TRUE(p.isBackSide(math::Vec3f(0.0f, y - 0.1f, 0.0f)));
	EXPECT_TRUE(p.isFrontSide(math::Vec3f(0.0f, y + 0.1f, 0.0f)));
}

TEST_F(PlaneTest, testWithNormalLeft) {
	const float x = 10.0;
	const Plane p(math::left(), math::Vec3f(x, 0.0f, 0.0f));
	EXPECT_TRUE(p.isFrontSide(math::Vec3f(0)));
	EXPECT_TRUE(p.isBackSide(math::Vec3f(x + 0.1f, 0.0f, 0.0f)));
	EXPECT_TRUE(p.isFrontSide(math::Vec3f(x - 0.1f, 0.0f, 0.0f)));
}

}
