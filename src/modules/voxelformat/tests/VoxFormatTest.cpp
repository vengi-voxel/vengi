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

TEST_F(VoxFormatTest, DISABLED_testSave) {
	VoxFormat f;
	const io::FilePtr& file = open("magicavoxel.vox");
	ASSERT_TRUE((bool)file) << "Could not open vox file";
	io::FileStream stream(file.get());
	RawVolume* loadedVolume = f.load(file->fileName(), stream);
	ASSERT_NE(nullptr, loadedVolume) << "Could not load vox file";

	const io::FilePtr& fileSave = open("magicavoxel-save.vox", io::FileMode::Write);
	io::FileStream sstream(fileSave.get());
	EXPECT_TRUE(f.save(loadedVolume, fileSave->fileName(), sstream));
	const io::FilePtr& fileLoadAfterSave = open("magicavoxel-save.vox");
	io::FileStream stream2(fileLoadAfterSave.get());
	RawVolume *savedVolume = f.load(fileLoadAfterSave->fileName(), stream2);
	EXPECT_NE(nullptr, savedVolume) << "Could not load saved vox file";
	if (savedVolume) {
		EXPECT_EQ(*savedVolume, *loadedVolume);
		delete savedVolume;
	}
	delete loadedVolume;
}

}
