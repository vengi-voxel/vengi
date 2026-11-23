/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "math/OBB.h"
#include "TestMathHelper.h"
#include "math/Mat3f.h"
#include "math/Vec3f.h"

namespace math {

class OBBTest : public app::AbstractTest {
};

TEST_F(OBBTest, testContains) {
	OBBF obb(math::Vec3f(0.0f), math::Vec3f(1.0f), math::Mat3f(1.0f));
	ASSERT_TRUE(obb.contains(math::Vec3f(0, 0, 0)));
	ASSERT_FALSE(obb.contains(math::Vec3f(1, 5, 1)));
}

TEST_F(OBBTest, testIntersects) {
	OBBF obb(math::Vec3f(0.0f), math::Vec3f(1.0f), math::Mat3f(1.0f));
	float distance;
	ASSERT_TRUE(obb.intersect(math::Vec3f(10.0f, 0.0, 0.0f), math::Vec3f(-1.0f, 0.0f, 0.0f), distance));
	ASSERT_FLOAT_EQ(distance, 9.0f);
}

}
