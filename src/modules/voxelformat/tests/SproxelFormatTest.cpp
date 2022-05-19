/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/SproxelFormat.h"

namespace voxelformat {

class SproxelFormatTest: public AbstractVoxFormatTest {
};

TEST_F(SproxelFormatTest, testLoadRGB) {
	testRGB("rgb.csv");
}

TEST_F(SproxelFormatTest, testSaveSmallVoxel) {
	SproxelFormat f;
	testSaveLoadVoxel("sproxel-smallvolumesavetest.csv", &f);
}

}
