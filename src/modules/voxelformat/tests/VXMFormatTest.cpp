/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXMFormat.h"

namespace voxel {

class VXMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXMFormatTest, testLoad) {
	VXMFormat f;
	std::unique_ptr<RawVolume> volume(load("test.vxm", f));
	ASSERT_NE(nullptr, volume) << "Could not load vxm file";
}

TEST_F(VXMFormatTest, testSaveVerySmallVoxel) {
	VXMFormat f;
	Region region(glm::ivec3(0), glm::ivec3(0, 1, 1));
	RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(0, 0, 1, createVoxel(VoxelType::Generic, 200)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 201)));
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 202)));
	const io::FilePtr &file = open("verysmallvolumesavetest.vxm", io::FileMode::Write);
	ASSERT_TRUE(f.save(&original, file));
	f = VXMFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("verysmallvolumesavetest.vxm")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(VXMFormatTest, DISABLED_testSaveSmallVoxel) {
	VXMFormat f;
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 245)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 127)));
	ASSERT_TRUE(original.setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, 200)));
	const io::FilePtr &file = open("smallvolumesavetest.vxm", io::FileMode::Write);
	ASSERT_TRUE(f.save(&original, file));
	f = VXMFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("smallvolumesavetest.vxm")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(VXMFormatTest, testSaveRLE) {
	VXMFormat f;
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(0, 0, 1, createVoxel(VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 127)));
	ASSERT_TRUE(original.setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, 127)));
	const io::FilePtr &file = open("smallvolumesavetest.vxm", io::FileMode::Write);
	ASSERT_TRUE(f.save(&original, file));
	f = VXMFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("smallvolumesavetest.vxm")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

}
