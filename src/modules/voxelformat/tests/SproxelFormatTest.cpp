/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/SproxelFormat.h"

namespace voxel {

class SproxelFormatTest: public AbstractVoxFormatTest {
};

TEST_F(SproxelFormatTest, testLoadRGB) {
	SproxelFormat f;
	std::unique_ptr<RawVolume> volume(load("rgb.csv", f));
	ASSERT_NE(nullptr, volume) << "Could not load sproxel csv file";
	testRGB(volume.get());
}

TEST_F(SproxelFormatTest, testSaveSmallVoxel) {
	SproxelFormat f;
	testSaveLoadVoxel("sproxel-smallvolumesavetest.csv", &f);
}

}
