/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/VoxFormat.h"

namespace voxel {

class VoxFormatTest: public AbstractVoxFormatTest {
};

TEST_F(VoxFormatTest, testLoad) {
	const io::FilePtr& file = _testApp->filesystem()->open("magicavoxel.vox");
	ASSERT_TRUE((bool)file) << "Could not open vox file";
	VoxFormat f;
	RawVolume* volume = f.load(file);
	ASSERT_NE(nullptr, volume) << "Could not load vox file";
	delete volume;
}

TEST_F(VoxFormatTest, testSave) {
	VoxFormat f;
	const io::FilePtr& file = _testApp->filesystem()->open("magicavoxel.vox");
	ASSERT_TRUE((bool)file) << "Could not open vox file";
	RawVolume* loadedVolume = f.load(file);
	ASSERT_NE(nullptr, loadedVolume) << "Could not load vox file";

	const io::FilePtr& fileSave = _testApp->filesystem()->open("magicavoxel-save.vox", io::FileMode::Write);
	EXPECT_TRUE(f.save(loadedVolume, fileSave));
	const io::FilePtr& fileLoadAfterSave = _testApp->filesystem()->open("magicavoxel-save.vox");
	RawVolume *savedVolume = f.load(fileLoadAfterSave);
	EXPECT_NE(nullptr, savedVolume) << "Could not load saved vox file";
	if (savedVolume) {
		EXPECT_EQ(*savedVolume, *loadedVolume);
		delete savedVolume;
	}
	delete loadedVolume;
}

}
