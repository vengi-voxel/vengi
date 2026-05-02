/**
 * @file
 */

#include "voxelformat/private/slab6/KVXFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class KVXFormatTest : public AbstractFormatTest {};

TEST_F(KVXFormatTest, testLoad) {
	testLoad("test.kvx");
}

TEST_F(KVXFormatTest, testSaveSmallVoxel) {
	KVXFormat f;
	testSaveLoadVoxel("kvx-smallvolumesavetest.kvx", &f, -16, 15,
					  voxel::ValidateFlags::AllPaletteMinMatchingColors);
}

} // namespace voxelformat
