/**
 * @file
 */

#include "voxel/tests/AbstractVoxelTest.h"
#include "voxelutil/VolumeCropper.h"

namespace voxel {

class VolumeCropperTest: public AbstractVoxelTest {
};

TEST_F(VolumeCropperTest, testCropSmall) {
	voxel::RawVolume smallVolume(voxel::Region(0, 2));
	smallVolume.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Grass, 0));
	RawVolume *croppedVolume = voxel::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), glm::ivec3(0)) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), glm::ivec3(0)) << croppedRegion.toString();
	EXPECT_EQ(croppedVolume->voxel(croppedRegion.getLowerCorner()), createVoxel(VoxelType::Grass, 0));
	delete croppedVolume;
}

TEST_F(VolumeCropperTest, testCropBigger) {
	voxel::Region region = voxel::Region(0, 100);
	voxel::RawVolume smallVolume(region);
	smallVolume.setVoxel(region.getCenter(), createVoxel(voxel::VoxelType::Grass, 0));
	voxel::RawVolume *croppedVolume = voxel::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_EQ(croppedVolume->voxel(region.getCenter()), createVoxel(VoxelType::Grass, 0)) << *croppedVolume;
	delete croppedVolume;
}

TEST_F(VolumeCropperTest, testCropBiggerMultiple) {
	voxel::Region region = voxel::Region(0, 100);
	voxel::RawVolume smallVolume(region);
	smallVolume.setVoxel(region.getCenter(), createVoxel(voxel::VoxelType::Grass, 0));
	smallVolume.setVoxel(region.getUpperCorner(), createVoxel(voxel::VoxelType::Grass, 0));
	voxel::RawVolume *croppedVolume = voxel::cropVolume(&smallVolume);
	ASSERT_NE(nullptr, croppedVolume) << "Expected to get the cropped raw volume";
	const voxel::Region& croppedRegion = croppedVolume->region();
	EXPECT_EQ(croppedRegion.getUpperCorner(), region.getUpperCorner()) << croppedRegion.toString();
	EXPECT_EQ(croppedRegion.getLowerCorner(), region.getCenter()) << croppedRegion.toString();
	EXPECT_EQ(croppedVolume->voxel(croppedRegion.getLowerCorner()), createVoxel(VoxelType::Grass, 0)) << *croppedVolume;
	EXPECT_EQ(croppedVolume->voxel(croppedRegion.getUpperCorner()), createVoxel(VoxelType::Grass, 0)) << *croppedVolume;
	delete croppedVolume;
}

}
