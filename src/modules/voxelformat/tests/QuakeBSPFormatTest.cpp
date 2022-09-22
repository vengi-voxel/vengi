/**
 * @file
 */

#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class QuakeBSPFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QuakeBSPFormatTest, DISABLED_testLoad) {
	canLoad("ufoai.bsp");
}

}
