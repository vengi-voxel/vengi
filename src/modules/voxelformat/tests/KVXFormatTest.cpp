/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/KVXFormat.h"

namespace voxel {

class KVXFormatTest: public AbstractVoxFormatTest {
};

TEST_F(KVXFormatTest, testLoad) {
	KVXFormat f;
	std::unique_ptr<RawVolume> volume(load("test.kvx", f));
}

}
