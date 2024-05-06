/**
 * @file
 */

#include "AbstractFormatTest.h"

namespace voxelformat {

class SMFormatTest : public AbstractFormatTest {};

TEST_F(SMFormatTest, testLoad) {
	// from fre starmade demo - allowed to use for testing purposes
	testLoad("Isanth_Type-Zero_Bc.sment", 1);
}

} // namespace voxelformat
