/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "noise/PoissonDiskDistribution.h"

namespace noise {

class PoissonDiskDistributionTest: public core::AbstractTest {
};

TEST_F(PoissonDiskDistributionTest, testAreaZeroOffset) {
	const core::RectFloat area(glm::vec2(0.0f), glm::vec2(128.0f));
	const std::vector<glm::vec2>& positions = noise::poissonDiskDistribution(15.0f, area);
	for (const glm::vec2& p : positions) {
		ASSERT_TRUE(area.contains(p));
	}
}

}
