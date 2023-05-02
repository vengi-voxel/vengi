/**
 * @file
 */

#include "voxel/Morton.h"
#include "app/tests/AbstractTest.h"

namespace voxel {

class MortonTest : public app::AbstractTest {};

TEST_F(MortonTest, testLookup) {
	const uint32_t idx = voxel::mortonIndex(5, 5, 5);
	uint8_t x, y, z;
	ASSERT_TRUE(voxel::mortonIndexToCoord(idx, x, y, z));
	ASSERT_EQ(5, x);
	ASSERT_EQ(5, y);
	ASSERT_EQ(5, z);
}

} // namespace voxel
