/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/GoxFormat.h"

namespace voxel {

class GoxFormatTest: public AbstractVoxFormatTest {
};

TEST_F(GoxFormatTest, testLoad) {
	GoxFormat f;
	std::unique_ptr<RawVolume> volume(load("test.gox", f));
	ASSERT_NE(nullptr, volume) << "Could not load volume";
}

TEST_F(GoxFormatTest, testSaveSmallVoxel) {
	GoxFormat f;
	testSaveLoadVoxel("goxel-smallvolumesavetest.gox", &f, -16, 15);
}

TEST_F(GoxFormatTest, testLoadRGB) {
	GoxFormat f;
	std::unique_ptr<RawVolume> volume(load("rgb.gox", f));
	ASSERT_NE(nullptr, volume) << "Could not load gox file";
	testRGB(volume.get());
}

}
