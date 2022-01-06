/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "voxel/Voxel.h"
#include "voxelformat/VoxFormat.h"

namespace voxel {

class VoxFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VoxFormatTest, testLoad) {
	VoxFormat f;
	std::unique_ptr<RawVolume> volume(load("magicavoxel.vox", f));
	ASSERT_NE(nullptr, volume) << "Could not load vox file";
}

TEST_F(VoxFormatTest, testLoadRGB) {
	VoxFormat f;
	std::unique_ptr<RawVolume> volume(load("rgb.vox", f));
	ASSERT_NE(nullptr, volume) << "Could not load vox file";
	testRGB(volume.get());
}

TEST_F(VoxFormatTest, testSaveSmallVoxel) {
	VoxFormat f;
	testSaveLoadVoxel("mv-smallvolumesavetest.vox", &f);
}

TEST_F(VoxFormatTest, testSaveMultipleLayers) {
	VoxFormat f;
	testSaveMultipleLayers("mv-multiplelayersavetest.vox", &f);
}

TEST_F(VoxFormatTest, testSaveBigVolume) {
	VoxFormat f;
	const Region region(glm::ivec3(0), glm::ivec3(1023, 0, 0));
	RawVolume bigVolume(region);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	bigVolume.setVoxel(0, 0, 0, voxel);
	bigVolume.setVoxel(256, 0, 0, voxel);
	bigVolume.setVoxel(512, 0, 0, voxel);
	const core::String name = "bigvolume.vox";
	ScopedSceneGraph sceneGraph;

#define VOX_TEST_SAVE_TO_FILE 0
#if VOX_TEST_SAVE_TO_FILE
	const io::FilePtr& filesave = open(name, io::FileMode::SysWrite);
	io::FileStream stream(filesave.get());
	const io::FilePtr& fileLoadAfterSave = open(name);
	io::FileStream streamread(fileLoadAfterSave.get());
	f.loadGroups(name, streamread, sceneGraph);
#else
	io::BufferedReadWriteStream stream(10 * 1024 * 1024);
	ASSERT_TRUE(f.save(&bigVolume, name, stream));
	stream.seek(0);
	f.loadGroups(name, stream, sceneGraph);
#endif
	EXPECT_EQ(3, (int)sceneGraph.size());
}

TEST_F(VoxFormatTest, testSave) {
	VoxFormat f;
	RawVolume* loadedVolume = load("magicavoxel.vox", f);
	ASSERT_NE(nullptr, loadedVolume) << "Could not load vox file";

	const io::FilePtr& fileSave = open("magicavoxel-save.vox", io::FileMode::Write);
	io::FileStream sstream(fileSave.get());
	EXPECT_TRUE(f.save(loadedVolume, fileSave->name(), sstream));
	const io::FilePtr& fileLoadAfterSave = open("magicavoxel-save.vox");
	io::FileStream stream2(fileLoadAfterSave.get());
	RawVolume *savedVolume = f.load(fileLoadAfterSave->name(), stream2);
	EXPECT_NE(nullptr, savedVolume) << "Could not load saved vox file";
	if (savedVolume) {
		EXPECT_EQ(*savedVolume, *loadedVolume);
		delete savedVolume;
	}
	delete loadedVolume;
}

}
