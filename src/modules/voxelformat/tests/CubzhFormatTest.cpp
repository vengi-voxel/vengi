/**
 * @file
 */

#include "voxelformat/private/cubzh/CubzhFormat.h"
#include "AbstractVoxFormatTest.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class CubzhFormatTest : public AbstractVoxFormatTest {};

TEST_F(CubzhFormatTest, testLoad) {
	canLoad("particubes.pcubes");
}

} // namespace voxelformat
