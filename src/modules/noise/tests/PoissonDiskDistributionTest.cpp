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
		ASSERT_TRUE(area.contains(p)) << p.x << ":" << p.y << " is not part of " << area.mins().x << ":"
									  << area.mins().y << "/" << area.maxs().x << ":" << area.maxs().y;
	}
	EXPECT_EQ(positions.size(), 51u);
}

TEST_F(PoissonDiskDistributionTest, testAABB) {
	const math::AABB<int> aabb(0, 0, 0, 64, 64, 64);
	const std::vector<glm::vec3>& positions = noise::poissonDiskDistribution(15.0f, aabb);
	for (const glm::vec3& p : positions) {
		ASSERT_TRUE(aabb.containsPoint(glm::ivec3(p)))
			<< p.x << ":" << p.y << ":" << p.z << " is not part of " << aabb.mins().x << ":" << aabb.mins().y << ":"
			<< aabb.mins().z << "/" << aabb.maxs().x << ":" << aabb.maxs().y << ":" << aabb.maxs().z;
	}
	EXPECT_EQ(positions.size(), 60u);
}

}
