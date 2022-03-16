/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/BinVoxFormat.h"

namespace voxelformat {

class BinVoxFormatTest: public AbstractVoxFormatTest {
};

TEST_F(BinVoxFormatTest, testLoad) {
	BinVoxFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("test.binvox", f));
	ASSERT_NE(nullptr, volume) << "Could not load binvox file";
}

TEST_F(BinVoxFormatTest, testSaveSmallVoxel) {
	BinVoxFormat f;
	testSaveLoadVoxel("bv-smallvolumesavetest.binvox", &f);
}

}
