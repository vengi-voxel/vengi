/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/KVXFormat.h"

namespace voxelformat {

class KVXFormatTest: public AbstractVoxFormatTest {
};

TEST_F(KVXFormatTest, testLoad) {
	canLoad("test.kvx");
}

TEST_F(KVXFormatTest, DISABLED_testSaveSmallVoxel) {
	KVXFormat f;
	testSaveLoadVoxel("kvx-smallvolumesavetest.kvx", &f, -16, 15);
}

}
