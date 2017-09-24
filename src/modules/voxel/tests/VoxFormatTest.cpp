/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/model/VoxFormat.h"

namespace voxel {

class VoxFormatTest: public AbstractVoxelTest {
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
	const io::FilePtr& file = _testApp->filesystem()->open("magicavoxel.vox");
	ASSERT_TRUE((bool)file) << "Could not open vox file";
	VoxFormat f;
	RawVolume* loadedVolume = f.load(file);
	ASSERT_NE(nullptr, loadedVolume) << "Could not load vox file";

	const io::FilePtr& fileSave = _testApp->filesystem()->open("magicavoxel-save.vox", io::FileMode::Write);
	ASSERT_TRUE(f.save(loadedVolume, fileSave));

	RawVolume *savedVolume = f.load(fileSave);
	ASSERT_NE(nullptr, savedVolume) << "Could not load saved vox file";

	EXPECT_EQ(*savedVolume, *loadedVolume);

	delete loadedVolume;
	delete savedVolume;
}

}
