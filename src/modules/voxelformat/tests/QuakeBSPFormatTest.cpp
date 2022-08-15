/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QuakeBSPFormat.h"

namespace voxelformat {

class QuakeBSPFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QuakeBSPFormatTest, DISABLED_testLoad) {
	canLoad("ufoai.bsp");
}

}
