#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "voxelformat/VolumeFormat.h"

namespace voxel {

const voxel::Voxel AbstractVoxFormatTest::Empty;

void AbstractVoxFormatTest::testLoadSaveAndLoad(const core::String& srcFilename, voxel::Format &srcFormat, const core::String& destFilename, voxel::Format &destFormat, bool includingColor, bool includingRegion) {
	std::unique_ptr<RawVolume> src(load(srcFilename, srcFormat));
	io::BufferedReadWriteStream stream(10 * 1024 * 1024);
	EXPECT_TRUE(destFormat.save(src.get(), destFilename, stream)) << "Could not save " << destFilename;
	stream.seek(0);
	std::unique_ptr<RawVolume> loaded(destFormat.load(destFilename, stream));
	ASSERT_NE(nullptr, loaded) << "Could not load " << destFilename;
	if (includingRegion) {
		ASSERT_EQ(src->region(), loaded->region());
	}
	EXPECT_TRUE(volumeComparator(*src.get(), *loaded, includingColor, includingRegion)) << "Volumes differ: " << *src.get() << *loaded;
}

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
	ScopedVoxelVolumes volumesLoad;
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file.get());
	EXPECT_TRUE(format->loadGroups(file->name(), stream, volumesLoad));
	EXPECT_EQ(volumesLoad.size(), volumes.size());
}

void AbstractVoxFormatTest::testSaveLoadVoxel(const core::String &filename, voxel::Format *format, int mins, int maxs) {
	const Region region(mins, maxs);
	RawVolume original(region);

	original.setVoxel(mins, mins, mins, createVoxel(VoxelType::Generic, 1));
	original.setVoxel(mins, mins, maxs, createVoxel(VoxelType::Generic, 245));
	original.setVoxel(mins, maxs, maxs, createVoxel(VoxelType::Generic, 127));
	original.setVoxel(mins, maxs, mins, createVoxel(VoxelType::Generic, 200));

	original.setVoxel(maxs, maxs, maxs, createVoxel(VoxelType::Generic, 1));
	original.setVoxel(maxs, maxs, mins, createVoxel(VoxelType::Generic, 245));
	original.setVoxel(maxs, mins, mins, createVoxel(VoxelType::Generic, 127));
	original.setVoxel(maxs, mins, maxs, createVoxel(VoxelType::Generic, 200));

	io::BufferedReadWriteStream stream(10 * 1024 * 1024);
	ASSERT_TRUE(format->save(&original, filename, stream));
	stream.seek(0);
	std::unique_ptr<RawVolume> loaded(format->load(filename, stream));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

} // namespace voxel
