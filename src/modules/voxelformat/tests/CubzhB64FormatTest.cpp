/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class CubzhB64FormatTest : public AbstractFormatTest {};

TEST_F(CubzhB64FormatTest, testLoad) {
	testLoad("hubmap.b64");
}

} // namespace voxelformat
