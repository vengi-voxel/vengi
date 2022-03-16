/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/KV6Format.h"

namespace voxelformat {

class KV6FormatTest: public AbstractVoxFormatTest {
};

TEST_F(KV6FormatTest, testLoad) {
	KV6Format f;
	std::unique_ptr<voxel::RawVolume> volume(load("test.kv6", f));
	ASSERT_NE(nullptr, volume) << "Could not load volume";
}

}
