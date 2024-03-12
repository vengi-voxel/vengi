/**
 * @file
 */

#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class SMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(SMFormatTest, testLoad) {
	// from fre starmade demo - allowed to use for testing purposes
	canLoad("Isanth_Type-Zero_Bc.sment", 1);
}

}
