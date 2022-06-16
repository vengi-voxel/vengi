/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/MCRFormat.h"
#include "voxelformat/QBFormat.h"

namespace voxelformat {

class MCRFormatTest: public AbstractVoxFormatTest {
};

TEST_F(MCRFormatTest, testLoad117) {
	canLoad("r.0.-2.mca",  128);
}

TEST_F(MCRFormatTest, testLoad110) {
	canLoad("minecraft_110.mca", 1024);
}

TEST_F(MCRFormatTest, testLoad113) {
	canLoad("minecraft_113.mca", 1024);
}

}
