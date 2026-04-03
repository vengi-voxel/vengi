/**
 * @file
 */

#include "voxelutil/VolumeSelect.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxelutil {

class VolumeSelectTest : public app::AbstractTest {
};

TEST_F(VolumeSelectTest, testRegionForFlagEmpty) {
	voxel::Region region(0, 9);
	voxel::RawVolume v(region);
	const voxel::Region result = regionForFlag(v, voxel::FlagOutline);
	EXPECT_FALSE(result.isValid());
}

TEST_F(VolumeSelectTest, testRegionForFlagNoMatch) {
	voxel::Region region(0, 9);
	voxel::RawVolume v(region);
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	v.setVoxel(5, 5, 5, voxel);
	const voxel::Region result = regionForFlag(v, voxel::FlagOutline);
	EXPECT_FALSE(result.isValid());
}

TEST_F(VolumeSelectTest, testRegionForFlagSingleVoxel) {
	voxel::Region region(0, 9);
	voxel::RawVolume v(region);
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel.setOutline();
	v.setVoxel(5, 5, 5, voxel);
	const voxel::Region result = regionForFlag(v, voxel::FlagOutline);
	ASSERT_TRUE(result.isValid());
	EXPECT_EQ(result.getLowerCorner(), glm::ivec3(5));
	EXPECT_EQ(result.getUpperCorner(), glm::ivec3(5));
}

TEST_F(VolumeSelectTest, testRegionForFlagMultipleVoxels) {
	voxel::Region region(0, 9);
	voxel::RawVolume v(region);
	voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	voxel.setOutline();
	v.setVoxel(2, 3, 4, voxel);
	v.setVoxel(7, 8, 6, voxel);
	const voxel::Region result = regionForFlag(v, voxel::FlagOutline);
	ASSERT_TRUE(result.isValid());
	EXPECT_EQ(result.getLowerCorner(), glm::ivec3(2, 3, 4));
	EXPECT_EQ(result.getUpperCorner(), glm::ivec3(7, 8, 6));
}

} // namespace voxelutil
