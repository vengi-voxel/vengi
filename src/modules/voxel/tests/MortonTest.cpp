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

TEST_F(MortonTest, testDecodeUint32) {
	// Morton code with bits interleaved as: x in bit positions 2,5,8,...; y in 1,4,7,...; z in 0,3,6,...
	// For coord (1,1,1): x=1 at bit2, y=1 at bit1, z=1 at bit0 => code = 0b111 = 7
	uint32_t x, y, z;
	voxel::mortonIndexToCoord(7u, x, y, z);
	EXPECT_EQ(1u, x);
	EXPECT_EQ(1u, y);
	EXPECT_EQ(1u, z);

	voxel::mortonIndexToCoord(0u, x, y, z);
	EXPECT_EQ(0u, x);
	EXPECT_EQ(0u, y);
	EXPECT_EQ(0u, z);
}

TEST_F(MortonTest, testEncodeDecodeRoundTrip) {
	for (uint32_t x = 0; x < 256; x += 37) {
		for (uint32_t y = 0; y < 256; y += 37) {
			for (uint32_t z = 0; z < 256; z += 37) {
				uint32_t code = voxel::mortonEncode(x, y, z);
				uint32_t dx, dy, dz;
				voxel::mortonIndexToCoord(code, dx, dy, dz);
				EXPECT_EQ(x, dx);
				EXPECT_EQ(y, dy);
				EXPECT_EQ(z, dz);
			}
		}
	}
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
