/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/BinVoxFormat.h"

namespace voxel {

class BinVoxFormatTest: public AbstractVoxFormatTest {
};

TEST_F(BinVoxFormatTest, testLoad) {
	BinVoxFormat f;
	std::unique_ptr<RawVolume> volume(load("test.binvox", f));
	ASSERT_NE(nullptr, volume) << "Could not load binvox file";
}

}
