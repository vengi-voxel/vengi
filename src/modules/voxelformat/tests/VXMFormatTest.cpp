/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VXMFormat.h"

namespace voxelformat {

class VXMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VXMFormatTest, DISABLED_testLoadRGB) {
	VXMFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("rgb.vxm", f));
	ASSERT_NE(nullptr, volume) << "Could not load vxm file";
	testRGB(volume.get());
}

TEST_F(VXMFormatTest, testLoad) {
	VXMFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("test.vxm", f));
	ASSERT_NE(nullptr, volume) << "Could not load vxm file";
}

TEST_F(VXMFormatTest, testLoadFileCreatedBySandboxVoxeditVersion12) {
	VXMFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("test2.vxm", f));
	ASSERT_NE(nullptr, volume) << "Could not load vxm file";
}

TEST_F(VXMFormatTest, testSaveVerySmallVoxel) {
	VXMFormat f;
	voxel::Region region(glm::ivec3(0), glm::ivec3(0, 1, 1));
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(0, 0, 1, createVoxel(voxel::VoxelType::Generic, 200)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(voxel::VoxelType::Generic, 201)));
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 202)));
	const io::FilePtr &file = open("verysmallvolumesavetest.vxm", io::FileMode::SysWrite);
	io::FileStream stream(file);
	ASSERT_TRUE(f.save(&original, file->name(), stream));
	f = VXMFormat();
	std::unique_ptr<voxel::RawVolume> loaded(load("verysmallvolumesavetest.vxm", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(VXMFormatTest, testSaveSmallVoxel) {
	VXMFormat f;
	testSaveLoadVoxel("sandbox-smallvolumesavetest.vxm", &f);
}

TEST_F(VXMFormatTest, testSaveRLE) {
	VXMFormat f;
	voxel::Region region(glm::ivec3(0), glm::ivec3(1));
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(0, 0, 1, createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(voxel::VoxelType::Generic, 127)));
	ASSERT_TRUE(original.setVoxel(0, 1, 0, createVoxel(voxel::VoxelType::Generic, 127)));
	const io::FilePtr &file = open("smallvolumesavetest.vxm", io::FileMode::SysWrite);
	io::FileStream stream(file);
	ASSERT_TRUE(f.save(&original, file->name(), stream));
	f = VXMFormat();
	std::unique_ptr<voxel::RawVolume> loaded(load("smallvolumesavetest.vxm", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

}
