/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/CSMFormat.h"

namespace voxelformat {

class CSMFormatTest: public AbstractVoxFormatTest {
};

TEST_F(CSMFormatTest, testLoad) {
	canLoad("chronovox-studio.csm", 11);
}

}
