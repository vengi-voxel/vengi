/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/FileStream.h"
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
