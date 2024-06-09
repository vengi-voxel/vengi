/**
 * @file
 */

#include "voxel/ModificationRecorder.h"
#include "app/tests/AbstractTest.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <gtest/gtest.h>

namespace voxel {

class ModificationRecorderTest : public app::AbstractTest {};

TEST_F(ModificationRecorderTest, testRecord) {
	voxel::RawVolume v(voxel::Region(0, 4));
	voxel::ModificationRecorder recorder(v);
	voxel::Voxel voxel = createVoxel(voxel::VoxelType::Generic, 1);
	ASSERT_TRUE(recorder.setVoxel(glm::ivec3(1, 1, 0), voxel));
	ASSERT_TRUE(recorder.setVoxel(glm::ivec3(3, 2, 0), voxel));
	EXPECT_TRUE(isAir(v.voxel(glm::ivec3(1, 1, 0)).getMaterial()));
	EXPECT_TRUE(isAir(v.voxel(glm::ivec3(3, 2, 0)).getMaterial()));
	const voxel::Region region = recorder.dirtyRegion();
	EXPECT_EQ(1, region.getLowerX());
	EXPECT_EQ(3, region.getUpperX());
	EXPECT_EQ(1, region.getLowerY());
	EXPECT_EQ(2, region.getUpperY());
	EXPECT_EQ(0, region.getLowerZ());
	EXPECT_EQ(0, region.getUpperZ());
}

} // namespace voxel
