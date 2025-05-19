/**
 * @file
 */

#include "voxelformat/private/binvox/BinVoxFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class BinVoxFormatTest : public AbstractFormatTest {};

TEST_F(BinVoxFormatTest, testLoad) {
	testLoad("test.binvox");
}

TEST_F(BinVoxFormatTest, testSaveSmallVoxel) {
	BinVoxFormat f;
	// binvox doesn't have colors, but only stores palette indices without the color
	// information.
	const voxel::ValidateFlags flags =
		voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Color | voxel::ValidateFlags::Palette);
	testSaveLoadVoxel("bv-smallvolumesavetest.binvox", &f, 0, 1, flags);
}

} // namespace voxelformat
