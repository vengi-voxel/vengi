/**
 * @file
 */

#include "math/Tri.h"
#include "app/tests/AbstractTest.h"
#include "math/Vec3f.h"

namespace math {

class TriTest : public app::AbstractTest {};

TEST_F(TriTest, testMinsMaxs) {
	Tri tri;
	tri.setVertices(math::Vec3f(-20, -10, -23), math::Vec3f(-10, -30, 23), math::Vec3f(20, 30, 40));
	const math::Vec3f mins = tri.mins();
	const math::Vec3f maxs = tri.maxs();
	EXPECT_FLOAT_EQ(-20.0f, mins.x);
	EXPECT_FLOAT_EQ(-30.0f, mins.y);
	EXPECT_FLOAT_EQ(-23.0f, mins.z);
	EXPECT_FLOAT_EQ(20.0f, maxs.x);
	EXPECT_FLOAT_EQ(30.0f, maxs.y);
	EXPECT_FLOAT_EQ(40.0f, maxs.z);
}

TEST_F(TriTest, testFlat) {
	Tri tri;
	tri.setVertices(math::Vec3f(0, 0, 0), math::Vec3f(1, 0, 0), math::Vec3f(0, 0, 1));
	EXPECT_TRUE(tri.flat()) << tri.normal().x << ":" << tri.normal().y << ":" << tri.normal().z;
	tri.setVertices(math::Vec3f(0, 0, 0), math::Vec3f(1, 1, 0), math::Vec3f(0, 0, 1));
	EXPECT_FALSE(tri.flat()) << tri.normal().x << ":" << tri.normal().y << ":" << tri.normal().z;
}

} // namespace math
