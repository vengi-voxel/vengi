/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxelutil/VolumeMerger.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxelutil {

class VolumeMergerTest: public app::AbstractTest {
};

TEST_F(VolumeMergerTest, testMergeDifferentSize) {
	voxel::RawVolume smallVolume(voxel::Region(0, 1));
	const voxel::Voxel vox = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	ASSERT_TRUE(smallVolume.setVoxel(0, 0, 0, vox));

	const voxel::Region region(0, 10);
	voxel::RawVolume bigVolume(voxel::Region(0, 10));
	const glm::ivec3 mergedPos = glm::ivec3(5);
	const voxel::Region& srcRegion = smallVolume.region();
	const voxel::Region destRegion(mergedPos, mergedPos + srcRegion.getUpperCorner());
	EXPECT_EQ(1, voxelutil::mergeVolumes(&bigVolume, &smallVolume, destRegion, srcRegion))
		<< "The single voxel from the small volume should have been merged into the big volume";

	const int32_t lowerX = region.getLowerX();
	const int32_t lowerY = region.getLowerY();
	const int32_t lowerZ = region.getLowerZ();
	const int32_t upperX = region.getUpperX();
	const int32_t upperY = region.getUpperY();
	const int32_t upperZ = region.getUpperZ();
	for (int32_t z = lowerZ; z <= upperZ; ++z) {
		for (int32_t y = lowerY; y <= upperY; ++y) {
			for (int32_t x = lowerX; x <= upperX; ++x) {
				const glm::ivec3 pos(x, y, z);
				if (pos == mergedPos) {
					EXPECT_TRUE(bigVolume.voxel(pos).isSame(vox));
				} else {
					EXPECT_FALSE(bigVolume.voxel(pos).isSame(vox));
				}
			}
		}
	}
}

TEST_F(VolumeMergerTest, testOffsets) {
	voxel::Region regionBig = voxel::Region(0, 5);
	voxel::Region regionSmall = voxel::Region(0, 3);
	voxel::RawVolume smallVolume(regionSmall);
	voxel::RawVolume bigVolume(regionBig);
	ASSERT_TRUE(bigVolume.setVoxel(regionBig.getCenter(), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(bigVolume.setVoxel(regionBig.getUpperCorner(), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	const voxel::Region srcRegion(regionBig.getCenter(), regionBig.getUpperCorner());
	const voxel::Region& destRegion = smallVolume.region();
	ASSERT_EQ(2, voxelutil::mergeVolumes(&smallVolume, &bigVolume, destRegion, srcRegion)) << smallVolume << ", " << bigVolume;
	ASSERT_TRUE(smallVolume.voxel(regionSmall.getLowerCorner()).isSame(voxel::createVoxel(voxel::VoxelType::Generic, 1))) << smallVolume << ", " << bigVolume;
	ASSERT_TRUE(smallVolume.voxel(regionSmall.getUpperCorner()).isSame(voxel::createVoxel(voxel::VoxelType::Generic, 1))) << smallVolume << ", " << bigVolume;
}

}
