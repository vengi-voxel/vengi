/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/VoxFormat.h"

namespace voxel {

class VoxFormatTest: public AbstractVoxelTest {
};

TEST_F(VoxFormatTest, testLoad) {
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open("magicavoxel.vox");
	ASSERT_TRUE((bool)file) << "Could not open vox file";
	VoxFormat f;
	RawVolume* volume = f.load(file);
	ASSERT_NE(nullptr, volume) << "Could not load vox file";
	delete volume;
}

TEST_F(VoxFormatTest, testSave) {
	// TODO:
}

}
