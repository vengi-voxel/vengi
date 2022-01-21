#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "voxelformat/VolumeFormat.h"

namespace voxel {

const voxel::Voxel AbstractVoxFormatTest::Empty;

void AbstractVoxFormatTest::testFirstAndLastPaletteIndex(const core::String &filename, voxel::Format *format, bool includingColor, bool includingRegion) {
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume volume(region);
	EXPECT_TRUE(volume.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 0)));
	EXPECT_TRUE(volume.setVoxel(0, 0, 1, createVoxel(VoxelType::Generic, 255)));
	io::BufferedReadWriteStream stream(10 * 1024 * 1024);
	ASSERT_TRUE(format->save(&volume, filename, stream));
	stream.seek(0);
	std::unique_ptr<RawVolume> loaded(format->load(filename, stream));
	ASSERT_NE(nullptr, loaded);
	EXPECT_TRUE(volumeComparator(volume, *loaded, false, includingRegion)) << "Volumes differ: " << volume << *loaded;
}

void AbstractVoxFormatTest::testFirstAndLastPaletteIndexConversion(voxel::Format &srcFormat, const core::String& destFilename, voxel::Format &destFormat, bool includingColor, bool includingRegion) {
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume original(region);
	EXPECT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 0)));
	EXPECT_TRUE(original.setVoxel(0, 0, 1, createVoxel(VoxelType::Generic, 255)));
	io::BufferedReadWriteStream srcFormatStream(10 * 1024 * 1024);
	EXPECT_TRUE(srcFormat.save(&original, destFilename, srcFormatStream)) << "Could not save " << destFilename;
	srcFormatStream.seek(0);
	std::unique_ptr<RawVolume> origReloaded(srcFormat.load(destFilename, srcFormatStream));
	if (includingRegion) {
		ASSERT_EQ(original.region(), origReloaded->region());
	}
	EXPECT_TRUE(volumeComparator(original, *origReloaded, includingColor, includingRegion)) << "Volumes differ: " << original << *origReloaded;

	io::BufferedReadWriteStream stream(10 * 1024 * 1024);
	EXPECT_TRUE(destFormat.save(origReloaded.get(), destFilename, stream)) << "Could not save " << destFilename;
	stream.seek(0);
	std::unique_ptr<RawVolume> loaded(destFormat.load(destFilename, stream));
	ASSERT_NE(nullptr, loaded) << "Could not load " << destFilename;
	if (includingRegion) {
		ASSERT_EQ(original.region(), loaded->region());
	}
	EXPECT_TRUE(volumeComparator(original, *loaded, includingColor, includingRegion)) << "Volumes differ: " << original << *loaded;
}

void AbstractVoxFormatTest::testRGB(RawVolume* volume) {
	EXPECT_EQ(VoxelType::Generic, volume->voxel( 0,  0,  0).getMaterial());
	EXPECT_EQ(VoxelType::Generic, volume->voxel(31,  0,  0).getMaterial());
	EXPECT_EQ(VoxelType::Generic, volume->voxel(31,  0, 31).getMaterial());
	EXPECT_EQ(VoxelType::Generic, volume->voxel( 0,  0, 31).getMaterial());

	EXPECT_EQ(VoxelType::Generic, volume->voxel( 0, 31,  0).getMaterial());
	EXPECT_EQ(VoxelType::Generic, volume->voxel(31, 31,  0).getMaterial());
	EXPECT_EQ(VoxelType::Generic, volume->voxel(31, 31, 31).getMaterial());
	EXPECT_EQ(VoxelType::Generic, volume->voxel( 0, 31, 31).getMaterial());

	EXPECT_EQ(VoxelType::Generic, volume->voxel( 9,  0,  4).getMaterial());
	EXPECT_EQ(VoxelType::Generic, volume->voxel( 9,  0, 12).getMaterial());
	EXPECT_EQ(VoxelType::Generic, volume->voxel( 9,  0, 19).getMaterial());

	EXPECT_EQ(245, volume->voxel( 0,  0,  0).getColor()) << "Expected to get the palette index 0";
	EXPECT_EQ(245, volume->voxel(31,  0,  0).getColor()) << "Expected to get the palette index 0";
	EXPECT_EQ(245, volume->voxel(31,  0, 31).getColor()) << "Expected to get the palette index 0";
	EXPECT_EQ(245, volume->voxel( 0,  0, 31).getColor()) << "Expected to get the palette index 0";

	EXPECT_EQ(  1, volume->voxel( 0, 31,  0).getColor()) << "Expected to get the palette index 1";
	EXPECT_EQ(  1, volume->voxel(31, 31,  0).getColor()) << "Expected to get the palette index 1";
	EXPECT_EQ(  1, volume->voxel(31, 31, 31).getColor()) << "Expected to get the palette index 1";
	EXPECT_EQ(  1, volume->voxel( 0, 31, 31).getColor()) << "Expected to get the palette index 1";

	EXPECT_EQ( 37, volume->voxel( 9,  0,  4).getColor()) << "Expected to get the palette index for red"; // red
	EXPECT_EQ(149, volume->voxel( 9,  0, 12).getColor()) << "Expected to get the palette index for green"; // green
	EXPECT_EQ(197, volume->voxel( 9,  0, 19).getColor()) << "Expected to get the palette index for blue"; // blue
}

void AbstractVoxFormatTest::testLoadSaveAndLoad(const core::String& srcFilename, voxel::Format &srcFormat, const core::String& destFilename, voxel::Format &destFormat, bool includingColor, bool includingRegion) {
	std::unique_ptr<RawVolume> src(load(srcFilename, srcFormat));
	ASSERT_TRUE(src.get() != nullptr);
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
	SceneGraph sceneGraph;
	voxel::SceneGraphNode node1;
	node1.setVolume(&layer1, false);
	voxel::SceneGraphNode node2;
	node2.setVolume(&layer2, false);
	voxel::SceneGraphNode node3;
	node3.setVolume(&layer3, false);
	voxel::SceneGraphNode node4;
	node4.setVolume(&layer4, false);
	sceneGraph.emplace(core::move(node1));
	sceneGraph.emplace(core::move(node2));
	sceneGraph.emplace(core::move(node3));
	sceneGraph.emplace(core::move(node4));
	const io::FilePtr &sfile = open(filename, io::FileMode::Write);
	io::FileStream sstream(sfile.get());
	ASSERT_TRUE(format->saveGroups(sceneGraph, sfile->name(), sstream));
	SceneGraph sceneGraphLoad;
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file.get());
	EXPECT_TRUE(format->loadGroups(file->name(), stream, sceneGraphLoad));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
}

void AbstractVoxFormatTest::testSave(const core::String &filename, voxel::Format *format) {
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume layer1(region);
	EXPECT_TRUE(layer1.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	SceneGraph sceneGraph;
	voxel::SceneGraphNode node1;
	node1.setVolume(&layer1, false);
	sceneGraph.emplace(core::move(node1));
	const io::FilePtr &sfile = open(filename, io::FileMode::Write);
	io::FileStream sstream(sfile.get());
	ASSERT_TRUE(format->saveGroups(sceneGraph, sfile->name(), sstream));
	SceneGraph sceneGraphLoad;
	const io::FilePtr &file = open(filename);
	io::FileStream stream(file.get());
	EXPECT_TRUE(format->loadGroups(file->name(), stream, sceneGraphLoad));
	EXPECT_EQ(sceneGraphLoad.size(), sceneGraph.size());
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
