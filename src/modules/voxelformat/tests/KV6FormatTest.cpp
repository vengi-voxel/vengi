/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/KV6Format.h"

namespace voxel {

class KV6FormatTest: public AbstractVoxFormatTest {
};

TEST_F(KV6FormatTest, testLoad) {
	KV6Format f;
	std::unique_ptr<RawVolume> volume(load("test.kv6", f));
}

}
