/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "math/AABB.h"

namespace math {

class AABBTest : public app::AbstractTest {
};

TEST_F(AABBTest, testContains) {
	AABB<int> aabb(0, 0, 0, 2, 2, 2);
	ASSERT_FALSE(aabb.isEmpty());
	ASSERT_TRUE(aabb.containsPoint(1, 1, 1));
	ASSERT_FALSE(aabb.containsPoint(1, 5, 1));
}

}
