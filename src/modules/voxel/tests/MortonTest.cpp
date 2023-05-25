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

TEST_F(MortonTest, testLookup256) {
	for (int x = 0; x < 256; x += 32) {
		for (int y = 0; y < 256; y += 32) {
			for (int z = 0; z < 256; z += 32) {
				const uint32_t idx = voxel::mortonIndex(x, y, z);
				uint8_t xx, yy, zz;
				ASSERT_TRUE(voxel::mortonIndexToCoord(idx, xx, yy, zz));
				EXPECT_EQ(x, xx);
				EXPECT_EQ(y, yy);
				EXPECT_EQ(z, zz);
			}
		}
	}
}

} // namespace voxel
