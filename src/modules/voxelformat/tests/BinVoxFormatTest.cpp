/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/BinVoxFormat.h"

namespace voxelformat {

class BinVoxFormatTest: public AbstractVoxFormatTest {
};

TEST_F(BinVoxFormatTest, testLoad) {
	canLoad("test.binvox");
}

TEST_F(BinVoxFormatTest, testSaveSmallVoxel) {
	BinVoxFormat f;
	testSaveLoadVoxel("bv-smallvolumesavetest.binvox", &f);
}

}
