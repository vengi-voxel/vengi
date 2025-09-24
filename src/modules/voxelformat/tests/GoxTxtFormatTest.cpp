/**
 * @file
 */

#include "voxelformat/private/goxel/GoxTxtFormat.h"
#include "AbstractFormatTest.h"

namespace voxelformat {

class GoxTxtFormatTest : public AbstractFormatTest {};

TEST_F(GoxTxtFormatTest, testLoad) {
	GoxTxtFormat f;
	testSaveLoadVoxel("goxel-smallvolumesavetest.txt", &f, 0, 15, voxel::ValidateFlags::None);
}

} // namespace voxelformat
