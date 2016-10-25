/**
 * @file
 */

#include "AbstractVoxelTest.h"
#include "voxel/VoxLoader.h"

namespace voxel {

class VoxLoaderTest: public AbstractVoxelTest {
};

TEST_F(VoxLoaderTest, testLoad) {
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open("magicavoxel.vox");
	ASSERT_TRUE((bool)file) << "Could not open vox file";
	VoxLoader loader;
	RawVolume* volume = loader.load(file);
	ASSERT_NE(nullptr, volume) << "Could not load vox file";
	delete volume;
}

}
