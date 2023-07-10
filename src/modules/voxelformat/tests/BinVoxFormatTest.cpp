/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/private/binvox/BinVoxFormat.h"

namespace voxelformat {

class BinVoxFormatTest: public AbstractVoxFormatTest {
};

TEST_F(BinVoxFormatTest, testLoad) {
	canLoad("test.binvox");
}

TEST_F(BinVoxFormatTest, testSaveSmallVoxel) {
	BinVoxFormat f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Color | voxel::ValidateFlags::Palette);
	testSaveLoadVoxel("bv-smallvolumesavetest.binvox", &f, 0, 1, flags);
}

}
