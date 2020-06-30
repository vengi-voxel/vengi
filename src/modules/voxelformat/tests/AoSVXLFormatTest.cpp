/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/AoSVXLFormat.h"

namespace voxel {

class AoSVXLFormatTest: public AbstractVoxFormatTest {
};

TEST_F(AoSVXLFormatTest, testLoad) {
	AoSVXLFormat f;
	std::unique_ptr<RawVolume> volume(load("aceofspades.vxl", f));
	ASSERT_NE(nullptr, volume) << "Could not load ace of spades file";
}

}
