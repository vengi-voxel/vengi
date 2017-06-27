/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/AABB.h"

namespace core {

class AABBTest : public core::AbstractTest {
};

TEST_F(AABBTest, testSplit) {
	glm::ivec3 mins(-64, -32, -32);
	glm::ivec3 maxs(64, 32, 96);
	core::AABB<int> aabb(mins, maxs);
	std::array<AABB<int>, 8> result;
	aabb.split(result);
	const glm::ivec3 center(0, 0, 32);

	ASSERT_EQ(result[0].mins(), mins);
	ASSERT_EQ(result[0].maxs(), center);

	ASSERT_EQ(result[1].mins(), glm::ivec3(mins.x, mins.y, center.z));
	ASSERT_EQ(result[1].maxs(), glm::ivec3(center.x, center.y, maxs.z));

	result[0].split(result);
	const glm::ivec3 center2(-32, -16, 0);
	ASSERT_EQ(result[0].mins(), mins);
	ASSERT_EQ(result[0].maxs(), center2);
}

}
