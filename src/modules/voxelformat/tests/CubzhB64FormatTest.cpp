/**
 * @file
 */

#include "voxelformat/private/cubzh/CubzhB64Format.h"
#include "AbstractVoxFormatTest.h"
#include "voxelformat/tests/TestHelper.h"

namespace voxelformat {

class CubzhB64FormatTest : public AbstractVoxFormatTest {};

TEST_F(CubzhB64FormatTest, testLoad) {
	canLoad("hubmap.b64");
}

} // namespace voxelformat
