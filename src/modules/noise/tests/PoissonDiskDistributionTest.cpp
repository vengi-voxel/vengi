/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "noise/PoissonDiskDistribution.h"

namespace noise {

class PoissonDiskDistributionTest: public app::AbstractTest {
};

TEST_F(PoissonDiskDistributionTest, testAreaZeroOffset) {
	const math::Rect<int> area(0, 0, 128, 128);
	const std::vector<glm::vec2>& positions = noise::poissonDiskDistribution(15.0f, area);
	for (const glm::vec2& p : positions) {
		ASSERT_TRUE(area.contains(p));
	}
	EXPECT_EQ(positions.size(), 49u);
}

TEST_F(PoissonDiskDistributionTest, testAreaOffset) {
	const math::Rect<int> area(128, 128, 256, 256);
	const std::vector<glm::vec2>& positions = noise::poissonDiskDistribution(15.0f, area);
	for (const glm::vec2& p : positions) {
		ASSERT_TRUE(area.contains(p)) << glm::to_string(p) << " is not part of " << glm::to_string(area.mins()) << "/" << glm::to_string(area.maxs());
	}
	EXPECT_EQ(positions.size(), 51u);
}

TEST_F(PoissonDiskDistributionTest, testAABB) {
	const math::AABB<int> aabb(0, 0, 0, 64, 64, 64);
	const std::vector<glm::vec3>& positions = noise::poissonDiskDistribution(15.0f, aabb);
	for (const glm::vec3& p : positions) {
		ASSERT_TRUE(aabb.containsPoint(glm::ivec3(p))) << glm::to_string(p) << " is not part of " << glm::to_string(aabb.mins()) << "/" << glm::to_string(aabb.maxs());
	}
	EXPECT_EQ(positions.size(), 60u);
}

}
