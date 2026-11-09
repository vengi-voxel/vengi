/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxelutil/VolumeCropper.h"
#include "voxel/tests/VoxelPrinter.h"

namespace voxelutil {

class VolumeCropperTest: public app::AbstractTest {
};

TEST_F(VolumeCropperTest, testChunkedBoundsDoNotMaterialize) {
	voxel::RawVolume volume(voxel::Region(-40, 216));
	const glm::ivec3 lower(-9, -8, -7);
	const glm::ivec3 upper(64, 63, 62);
	volume.setVoxel(lower, voxel::Voxel(voxel::VoxelType::Generic, 1));
	volume.setVoxel(upper, voxel::Voxel(voxel::VoxelType::Generic, 2));
	glm::ivec3 mins, maxs;
	ASSERT_TRUE(computeSolidBounds(&volume, mins, maxs));
	EXPECT_EQ(lower, mins);
	EXPECT_EQ(upper, maxs);
	EXPECT_EQ(nullptr, volume.linearVoxels());
	volume.clear();
	EXPECT_FALSE(computeSolidBounds(&volume, mins, maxs));
	EXPECT_EQ(nullptr, volume.linearVoxels());
}

TEST_F(VolumeCropperTest, testBoundsIgnoreAirMetadataInBothStorageModes) {
	const int sides[] = {3, 257};
	for (const int side : sides) {
		voxel::RawVolume volume(voxel::Region(-40, side - 41));
		volume.fill(voxel::Voxel(voxel::VoxelType::Air, 12, 9, voxel::FlagOutline, 3));
		const size_t bytes = volume.allocatedBytes();
		glm::ivec3 mins, maxs;
		EXPECT_FALSE(computeSolidBounds(&volume, mins, maxs));
		EXPECT_EQ(bytes, volume.allocatedBytes());
		const glm::ivec3 center = volume.region().getCenter();
		volume.setVoxel(center, voxel::Voxel(voxel::VoxelType::Generic, 2));
		const size_t markedBytes = volume.allocatedBytes();
		ASSERT_TRUE(computeSolidBounds(&volume, mins, maxs));
		EXPECT_EQ(center, mins);
		EXPECT_EQ(center, maxs);
		EXPECT_EQ(markedBytes, volume.allocatedBytes());
		EXPECT_EQ(side == 3, volume.isContiguous());
	}
}

TEST_F(VolumeCropperTest, testCropSmall) {
	voxel::RawVolume smallVolume(voxel::Region(0, 2));
	smallVolume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume *croppedVolume = voxelutil::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), glm::ivec3(0)) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), glm::ivec3(0)) << croppedRegion.toString();
	EXPECT_TRUE(croppedVolume->voxel(croppedRegion.getLowerCorner()).isSame(voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	delete croppedVolume;
}

TEST_F(VolumeCropperTest, testCropBigger) {
	voxel::Region region = voxel::Region(0, 100);
	voxel::RawVolume smallVolume(region);
	smallVolume.setVoxel(region.getCenter(), voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume *croppedVolume = voxelutil::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_TRUE(croppedVolume->voxel(region.getCenter()).isSame(voxel::createVoxel(voxel::VoxelType::Generic, 1))) << *croppedVolume;
	delete croppedVolume;
}

TEST_F(VolumeCropperTest, testCropBiggerMultiple) {
	voxel::Region region = voxel::Region(0, 100);
	voxel::RawVolume smallVolume(region);
	smallVolume.setVoxel(region.getCenter(), voxel::createVoxel(voxel::VoxelType::Generic, 1));
	smallVolume.setVoxel(region.getUpperCorner(), voxel::createVoxel(voxel::VoxelType::Generic, 1));
	voxel::RawVolume *croppedVolume = voxelutil::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), region.getUpperCorner()) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_TRUE(croppedVolume->voxel(croppedRegion.getLowerCorner()).isSame(voxel::createVoxel(voxel::VoxelType::Generic, 1))) << *croppedVolume;
	EXPECT_TRUE(croppedVolume->voxel(croppedRegion.getUpperCorner()).isSame(voxel::createVoxel(voxel::VoxelType::Generic, 1))) << *croppedVolume;
	delete croppedVolume;
}

}
