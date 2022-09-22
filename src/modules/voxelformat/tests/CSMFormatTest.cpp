/**
 * @file
 */

#include "AbstractVoxFormatTest.h"

namespace voxelformat {

class CSMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(CSMFormatTest, testLoad) {
	canLoad("chronovox-studio.csm", 11);
}

}
