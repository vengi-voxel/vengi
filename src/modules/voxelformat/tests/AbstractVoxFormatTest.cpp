#include "AbstractVoxFormatTest.h"
#include "voxelformat/VolumeFormat.h"

namespace voxel {

const voxel::Voxel AbstractVoxFormatTest::Empty;

void AbstractVoxFormatTest::testSaveMultipleLayers(const core::String &filename, voxel::Format *format) {
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume layer1(region);
	RawVolume layer2(region);
	RawVolume layer3(region);
	RawVolume layer4(region);
	EXPECT_TRUE(layer1.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer2.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer3.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer4.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	VoxelVolumes volumes;
	volumes.push_back(VoxelVolume(&layer1));
	volumes.push_back(VoxelVolume(&layer2));
	volumes.push_back(VoxelVolume(&layer3));
	volumes.push_back(VoxelVolume(&layer4));
	const io::FilePtr &sfile = open(filename, io::FileMode::Write);
	io::FileStream sstream(sfile.get());
	ASSERT_TRUE(format->saveGroups(volumes, sfile->name(), sstream));
	VoxelVolumes volumesLoad;
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file.get());
	EXPECT_TRUE(format->loadGroups(file->name(), stream, volumesLoad));
	EXPECT_EQ(volumesLoad.size(), volumes.size());
	voxelformat::clearVolumes(volumesLoad);
}

void AbstractVoxFormatTest::testSaveLoadVoxel(const core::String &filename, voxel::Format *format) {
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume original(region);
	original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1));
	original.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 245));
	original.setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 127));
	original.setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, 200));
	const io::FilePtr &file = open(filename, io::FileMode::Write);
	io::FileStream stream(file.get());
	ASSERT_TRUE(format->save(&original, file->name(), stream));
	std::unique_ptr<RawVolume> loaded(load(filename, *format));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

} // namespace voxel
