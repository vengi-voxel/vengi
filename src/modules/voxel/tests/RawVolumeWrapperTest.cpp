/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxel {

class RawVolumeWrapperTest: public app::AbstractTest {
};

TEST_F(RawVolumeWrapperTest, testSetOneVoxelVolume) {
	Region region(0, 0);
	RawVolume v(region);
	RawVolumeWrapper w(&v);
	EXPECT_TRUE(w.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 0)));
}

TEST_F(RawVolumeWrapperTest, testSetVoxelInside) {
	Region region(0, 7);
	RawVolume v(region);
	RawVolumeWrapper w(&v);
	EXPECT_TRUE(w.setVoxel(3, 4, 3, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_EQ(w.dirtyRegion(), voxel::Region(3, 4, 3, 3, 4, 3));
}

TEST_F(RawVolumeWrapperTest, testSetMinBoundary) {
	Region region(0, 7);
	RawVolume v(region);
	RawVolumeWrapper w(&v);
	EXPECT_TRUE(w.setVoxel(0, 0, 0, voxel::createVoxel(VoxelType::Generic, 01)));
	EXPECT_FALSE(w.setVoxel(-1, -1, -1, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_FALSE(w.setVoxel(0, 0, -1, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_FALSE(w.setVoxel(0, -1, 0, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_FALSE(w.setVoxel(-1, 0, 0, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_EQ(w.dirtyRegion(), voxel::Region(0, 0));
}

TEST_F(RawVolumeWrapperTest, testSetMaxBoundary) {
	Region region(0, 7);
	RawVolume v(region);
	RawVolumeWrapper w(&v);
	EXPECT_TRUE(w.setVoxel(7, 7, 7, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_FALSE(w.setVoxel(8, 8, 8, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_FALSE(w.setVoxel(7, 7, 8, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_FALSE(w.setVoxel(7, 8, 7, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_FALSE(w.setVoxel(8, 7, 7, voxel::createVoxel(VoxelType::Generic, 0)));
	EXPECT_EQ(w.dirtyRegion(), voxel::Region(7, 7));
}

TEST_F(RawVolumeWrapperTest, testSampler) {
	Region region(0, 7);
	RawVolume v(region);
	RawVolumeWrapper w(&v);
	RawVolumeWrapper::Sampler sampler(&w);
	sampler.setPosition(3, 4, 3);
	EXPECT_TRUE(sampler.setVoxel(voxel::createVoxel(VoxelType::Generic, 0)));
	sampler.movePositiveX();
	EXPECT_TRUE(sampler.setVoxel(voxel::createVoxel(VoxelType::Generic, 0)));
	sampler.flush();
	EXPECT_EQ(w.dirtyRegion(), voxel::Region(3, 4, 3, 4, 4, 3));

}

}
