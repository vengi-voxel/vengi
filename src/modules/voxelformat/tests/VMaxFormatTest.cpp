/**
 * @file
 */

#include "voxelformat/VMaxFormat.h"
#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class VMaxFormatTest : public AbstractVoxFormatTest {};

TEST_F(VMaxFormatTest, DISABLED_testLoad) {
	canLoad("voxelmax.vmax.zip");
}

} // namespace voxelformat
