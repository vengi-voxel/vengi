/**
 * @file
 */

#include "voxel/SparseVolume.h"
#include "AbstractVoxelTest.h"
#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxel {

class SparseVolumeTest : public app::AbstractTest {};

TEST_F(SparseVolumeTest, testSetVoxels) {
	SparseVolume v(voxel::Region(0, 10));
	ASSERT_EQ(0u, v.size());
	ASSERT_TRUE(v.empty());
	ASSERT_TRUE(v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(1u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_TRUE(v.setVoxel(10, 10, 10, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(2u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_FALSE(v.setVoxel(11, 11, 11, voxel::createVoxel(VoxelType::Generic, 0)));
	ASSERT_EQ(2u, v.size());
	ASSERT_FALSE(v.empty());
	ASSERT_TRUE(v.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Air, 0)));
	ASSERT_EQ(1u, v.size());
	ASSERT_FALSE(v.empty());
}

} // namespace voxel
